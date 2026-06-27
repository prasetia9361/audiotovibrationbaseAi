#include "SampleGetter.h"
#include "config.h"

// ================================================================
// SampleGetter — Implementasi
// ================================================================

// ---- WAV Header (44 byte, Little Endian) ----
// Digunakan untuk membungkus raw PCM 16-bit mono menjadi file WAV
// yang bisa langsung dikirim ke Edge Impulse Ingestion API.
static void writeU16LE(uint8_t* buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}
static void writeU32LE(uint8_t* buf, uint32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8)  & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

// ================================================================

SampleGetter::SampleGetter(): 
    _labelIndex(0),
    _sampleCount(0),
    _wifiConnected(false), 
    _btnLastState(HIGH), 
    _inputState(false),
    _isInputagain(true),
    _btnPressTime(0), 
    _longPressHandled(false)
{}

// ----------------------------------------------------------------
// begin()
// ----------------------------------------------------------------
bool SampleGetter::begin() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(800);

    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   SampleGetter — Edge Impulse        ║");
    Serial.println("║   Mode Pengambilan Data Audio        ║");
    Serial.println("╚══════════════════════════════════════╝");

    // LED
    pinMode(PIN_LED, OUTPUT);
    _ledOff();

    // Button (INPUT_PULLUP: LOW saat ditekan)
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // Mikrofon I2S
    if (!_audio.begin()) {
        Serial.println("[SampleGetter] ERROR: Mikrofon gagal init");
        _ledBlink(10, 100); // error cepat
        return false;
    }

    // WiFi
    if (!_connectWiFi()) {
        Serial.println("[SampleGetter] ERROR: WiFi gagal tersambung");
        _ledBlink(5, 300);
        return false;
    }

    // Abaikan verifikasi sertifikat SSL (cukup untuk dev)
    _wifiClient.setInsecure();

    _printStatus();
    Serial.println("[SampleGetter] Siap! Tekan tombol untuk merekam.");
    _ledBlink(3, 200);
    return true;
}

// ----------------------------------------------------------------
// task() — dipanggil terus dari loop()
// ----------------------------------------------------------------
void SampleGetter::task() {
    if (!_inputState)
    {
        if (_isInputagain) 
        {
            Serial.printf("Input serial: ketik angka 0 – %d lalu Enter\n", LABEL_COUNT - 1);
            _isInputagain = false;
        }
        // ---- Input serial: ketik angka 0–(LABEL_COUNT-1) lalu Enter ----
        if (Serial.available() > 0) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            if (input.length() > 0) {
                int idx = input.toInt();
                if (idx >= 0 && idx < LABEL_COUNT) {
                    _inputState = true;
                    _labelIndex = (uint8_t)idx;
                    Serial.printf("\n[SERIAL] Label diset → \"%s\" (index %d)\n",
                                SAMPLE_LABELS[_labelIndex], _labelIndex);
                    _printStatus();
                } else {
                    Serial.printf("[SERIAL] Index tidak valid. Masukkan angka 0–%d\n",
                                LABEL_COUNT - 1);
                    _isInputagain = true;
                }
            }
        }
    }else {
        bool btnNow = digitalRead(PIN_BUTTON);  // LOW = ditekan

        // ---- Deteksi tombol ditekan ----
        if (btnNow == LOW && _btnLastState == HIGH) {
            _btnPressTime    = millis();
            _longPressHandled = false;
        }

        // ---- Deteksi long press (tahan) ----
        if (btnNow == LOW && !_longPressHandled) {
            if ((millis() - _btnPressTime) >= BTN_LONG_PRESS_MS) {
                // _nextLabel();
                _inputState = false; // reset input serial
                _isInputagain = true;
                _longPressHandled = true;
            }
        }

        // ---- Deteksi tombol dilepas ----
        if (btnNow == HIGH && _btnLastState == LOW) {
            uint32_t duration = millis() - _btnPressTime;
            if (!_longPressHandled && duration >= BTN_DEBOUNCE_MS) {
                // Short press → rekam dan upload
                _recordAndUpload();
            }
        }

        _btnLastState = btnNow;

    }
}

// ----------------------------------------------------------------
// Private — Rekam dan upload ke Edge Impulse
// ----------------------------------------------------------------
void SampleGetter::_recordAndUpload() {
    Serial.printf("\n[REC] Label: \"%s\" — Merekam...\n", SAMPLE_LABELS[_labelIndex]);
    _ledOn();

    // 1. Rekam audio
    if (!_audio.capture()) {
        Serial.println("[REC] GAGAL: Error baca mikrofon");
        _ledBlink(5, 100);
        return;
    }
    Serial.println("[REC] Rekaman selesai (1 detik)");
    _ledOff();

    // 2. Upload ke Edge Impulse
    Serial.println("[UPLOAD] Mengirim ke Edge Impulse...");
    _ledBlink(2, 150); // kedip 2x saat upload

    bool ok = _uploadToEdgeImpulse(_audio.getBuffer(), _audio.getBufferSize());

    if (ok) {
        _sampleCount++;
        Serial.printf("[UPLOAD] Sukses! Total sesi ini: %d sampel\n", _sampleCount);
        _ledBlink(3, 100); // sukses: 3 kedip cepat
    } else {
        Serial.println("[UPLOAD] GAGAL — coba lagi");
        _ledBlink(5, 80);  // gagal: 5 kedip sangat cepat
    }
    _printStatus();
}

// ----------------------------------------------------------------
// Private — Ganti label aktif
// ----------------------------------------------------------------
void SampleGetter::_nextLabel() {
    _labelIndex = (_labelIndex + 1) % LABEL_COUNT;
    Serial.printf("\n[LABEL] Ganti ke → \"%s\"\n", SAMPLE_LABELS[_labelIndex]);
    _printStatus();
    // Kedip sesuai index label baru (1 kedip = noise, 2 = klakson, 3 = sirine)
    _ledBlink(_labelIndex + 1, 250);
}

// ----------------------------------------------------------------
// Private — Koneksi WiFi
// ----------------------------------------------------------------
bool SampleGetter::_connectWiFi() {
    Serial.printf("[WiFi] Menghubungkan ke \"%s\"", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if ((millis() - start) >= WIFI_TIMEOUT_MS) {
            Serial.println("\n[WiFi] Timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Tersambung! IP: %s\n", WiFi.localIP().toString().c_str());
    _wifiConnected = true;
    return true;
}

// ----------------------------------------------------------------
// Private — Build WAV header (44 byte)
// ----------------------------------------------------------------
void SampleGetter::_buildWavHeader(uint8_t* dst, uint32_t numSamples) {
    const uint16_t NUM_CHANNELS    = 1;
    const uint16_t BITS_PER_SAMPLE = 16;
    const uint32_t BYTE_RATE       = AUDIO_SAMPLE_RATE * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    const uint16_t BLOCK_ALIGN     = NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    const uint32_t DATA_SIZE       = numSamples * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);

    // RIFF chunk
    memcpy(dst +  0, "RIFF", 4);
    writeU32LE(dst +  4, 36 + DATA_SIZE);     // ChunkSize
    memcpy(dst +  8, "WAVE", 4);

    // fmt sub-chunk
    memcpy(dst + 12, "fmt ", 4);
    writeU32LE(dst + 16, 16);                 // SubchunkSize (PCM = 16)
    writeU16LE(dst + 20, 1);                  // AudioFormat  (PCM = 1)
    writeU16LE(dst + 22, NUM_CHANNELS);
    writeU32LE(dst + 24, AUDIO_SAMPLE_RATE);
    writeU32LE(dst + 28, BYTE_RATE);
    writeU16LE(dst + 32, BLOCK_ALIGN);
    writeU16LE(dst + 34, BITS_PER_SAMPLE);

    // data sub-chunk
    memcpy(dst + 36, "data", 4);
    writeU32LE(dst + 40, DATA_SIZE);
}

// ----------------------------------------------------------------
// Private — Upload ke Edge Impulse Ingestion API via HTTPS
// ----------------------------------------------------------------
bool SampleGetter::_uploadToEdgeImpulse(int16_t* pcmBuffer, size_t numSamples) {
    // Cek koneksi WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[UPLOAD] WiFi terputus, coba reconnect...");
        if (!_connectWiFi()) return false;
    }

    // Alokasi buffer WAV = 44 byte header + data PCM
    const size_t  pcmBytes  = numSamples * sizeof(int16_t);
    const size_t  wavSize   = 44 + pcmBytes;
    uint8_t*      wavBuffer = (uint8_t*)malloc(wavSize);
    if (!wavBuffer) {
        Serial.println("[UPLOAD] ERROR: malloc gagal (RAM tidak cukup)");
        return false;
    }

    // Tulis header lalu copy PCM
    _buildWavHeader(wavBuffer, (uint32_t)numSamples);
    memcpy(wavBuffer + 44, pcmBuffer, pcmBytes);

    // Kirim HTTP POST
    HTTPClient http;
    String url = String("https://") + EI_INGESTION_HOST + EI_INGESTION_URL;
    http.begin(_wifiClient, url);

    http.addHeader("x-api-key",           EI_API_KEY);
    http.addHeader("x-label",             SAMPLE_LABELS[_labelIndex]);
    http.addHeader("x-disallow-duplicates", "0");
    http.addHeader("Content-Type",        "audio/wav");

    int httpCode = http.POST(wavBuffer, wavSize);

    bool success = false;
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        success = true;
    } else {
        Serial.printf("[UPLOAD] HTTP error: %d — %s\n",
            httpCode, http.getString().c_str());
    }

    http.end();
    free(wavBuffer);
    return success;
}

// ----------------------------------------------------------------
// Private — LED
// ----------------------------------------------------------------
void SampleGetter::_ledOn()  { digitalWrite(PIN_LED, LOW);  } // XIAO: LOW = nyala
void SampleGetter::_ledOff() { digitalWrite(PIN_LED, HIGH); }

void SampleGetter::_ledBlink(uint8_t times, uint16_t periodMs) {
    for (uint8_t i = 0; i < times; i++) {
        _ledOn();
        delay(periodMs / 2);
        _ledOff();
        delay(periodMs / 2);
    }
}

// ----------------------------------------------------------------
// Private — Print status ke Serial
// ----------------------------------------------------------------
void SampleGetter::_printStatus() {
    Serial.println("┌──────────────────────────────┐");
    Serial.printf( "│ Label aktif : %-14s │\n", SAMPLE_LABELS[_labelIndex]);
    Serial.printf( "│ Upload OK   : %-4d sampel     │\n", _sampleCount);
    Serial.printf( "│ WiFi        : %-14s │\n",
        WiFi.status() == WL_CONNECTED ? "Tersambung" : "Terputus");
    Serial.println("├──────────────────────────────┤");
    Serial.println("│ [Tekan singkat] → Rekam      │");
    Serial.println("│ [Tahan 2 detik] → Ganti label│");
    Serial.println("└──────────────────────────────┘");
}
