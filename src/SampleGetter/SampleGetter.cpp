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
    _isDurationPrompted(false),
    _btnPressTime(0),
    _durationTime(0),
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

    _printStatus();
    Serial.println("[SampleGetter] Siap! Tekan tombol untuk merekam.");
    _ledBlink(3, 200);
    return true;
}

// ----------------------------------------------------------------
// task() — dipanggil terus dari loop()
// ----------------------------------------------------------------
void SampleGetter::task() {
    if (!_inputState) {
        // ---- Fase 1a: input label ----
        if (_isInputagain) {
            Serial.printf("[INPUT] Ketik index label (0–%d) lalu Enter:\n", LABEL_COUNT - 1);
            for (int i = 0; i < LABEL_COUNT; i++)
                Serial.printf("  %d = %s\n", i, SAMPLE_LABELS[i]);
            _isInputagain = false;
        }

        if (!_isDurationPrompted && Serial.available() > 0) {
            String input = "";
            while (Serial.available() > 0)
            {
                input = Serial.readStringUntil('\n');
                input.trim();
            }
            if (input.length() > 0) {
                int idx = input.toInt();
                if (idx >= 0 && idx < LABEL_COUNT) {
                    _labelIndex = (uint8_t)idx;
                    Serial.printf("[INPUT] Label → \"%s\". Ketik durasi rekam (100–%d ms) lalu Enter: ",
                                SAMPLE_LABELS[_labelIndex], SAMPLER_MAX_DURATION_MS);
                    _isDurationPrompted = true;
                } else {
                    Serial.printf("[INPUT] Index tidak valid. Masukkan angka 0–%d\n", LABEL_COUNT - 1);
                    _isInputagain = true;
                }
            }
        }

        // ---- Fase 1b: input durasi ----
        if (_isDurationPrompted && Serial.available() > 0) {
            String input = "";
            while (Serial.available() > 0)
            {
                input = Serial.readStringUntil('\n');
                input.trim();
            }
            if (input.length() > 0) {
                int dur = input.toInt();
                if (dur >= 100 && dur <= (int)SAMPLER_MAX_DURATION_MS) {
                    _durationTime = (float)dur;
                    _inputState   = true;
                    _printStatus();
                } else {
                    Serial.printf("[INPUT] Durasi tidak valid. Masukkan 100–%d ms: ", SAMPLER_MAX_DURATION_MS);
                }
            }
        }

    } else {
        bool btnNow = digitalRead(PIN_BUTTON);  // LOW = ditekan
        

        // ---- Deteksi tombol ditekan ----
        if (btnNow == LOW && _btnLastState == HIGH) {
            Serial.printf("[BUTTON] status %d \n", btnNow);
            _btnPressTime    = millis();
            _longPressHandled = false;
        }

        // ---- Deteksi long press (tahan) ----
        if (btnNow == LOW && !_longPressHandled) {
            if ((millis() - _btnPressTime) >= BTN_LONG_PRESS_MS) {
                Serial.printf("[BUTTON] Long press → ganti label\n");
                // _nextLabel();
                _inputState       = false;
                _isInputagain     = true;
                _isDurationPrompted = false;
                _longPressHandled = true;
            }
        }

        // ---- Deteksi tombol dilepas ----
        if (btnNow == HIGH && _btnLastState == LOW) {
            Serial.printf("[BUTTON] status %d \n", btnNow);
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
    Serial.printf("\n[REC] Label: \"%s\" — Merekam %.0f ms...\n",
                  SAMPLE_LABELS[_labelIndex], _durationTime);
    _ledOn();

    size_t duration = (size_t)(AUDIO_SAMPLE_RATE * _durationTime / 1000.0f);

    // Gunakan ps_malloc agar buffer dialokasikan dari PSRAM (8MB N16R8)
    _buffer = (int16_t*)ps_malloc(sizeof(int16_t) * duration);
    if (!_buffer) {
        Serial.println("[REC] GAGAL: malloc gagal (RAM tidak cukup)");
        _ledBlink(5, 100);
        return;
    }

    if (!_audio.capture(duration, _buffer)) {
        Serial.println("[REC] GAGAL: Error baca mikrofon");
        _ledBlink(5, 100);
        free(_buffer);
        _buffer = nullptr;
        return;
    }
    _ledOff();

    Serial.println("[UPLOAD] Mengirim ke Edge Impulse...");
    _ledBlink(2, 150);

    bool ok = _uploadToEdgeImpulse(_buffer, _audio.getBufferSize());

    free(_buffer);
    _buffer = nullptr;

    if (ok) {
        _sampleCount++;
        Serial.printf("[UPLOAD] Sukses! Total sesi ini: %d sampel\n", _sampleCount);
        _ledBlink(3, 100);
    } else {
        Serial.println("[UPLOAD] GAGAL — coba lagi");
        _ledBlink(5, 80);
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
    writeU32LE(dst +  4, 36 + DATA_SIZE);
    memcpy(dst +  8, "WAVE", 4);

    // fmt sub-chunk
    memcpy(dst + 12, "fmt ", 4);
    writeU32LE(dst + 16, 16);
    writeU16LE(dst + 20, 1);
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
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[UPLOAD] WiFi terputus, coba reconnect...");
        if (!_connectWiFi()) return false;
    }

    const size_t pcmBytes = numSamples * sizeof(int16_t);
    const size_t wavSize  = 44 + pcmBytes;

    String fileName   = String(SAMPLE_LABELS[_labelIndex]) + "." + _sampleCount + ".wav";
    const char* boundary = "EIBoundary";

    String partHeader = String("--") + boundary + "\r\n"
        + "Content-Disposition: form-data; name=\"data\"; filename=\"" + fileName + "\"\r\n"
        + "Content-Type: audio/wav\r\n\r\n";
    String partFooter = String("\r\n--") + boundary + "--\r\n";

    size_t totalBody = partHeader.length() + wavSize + partFooter.length();

    // Bangun HTTP request header (stack — kecil)
    String reqHeader = String("POST ") + EI_INGESTION_URL + " HTTP/1.1\r\n"
        + "Host: " + EI_INGESTION_HOST + "\r\n"
        + "x-api-key: " + EI_API_KEY + "\r\n"
        + "x-file-name: " + fileName + "\r\n"
        + "x-label: " + SAMPLE_LABELS[_labelIndex] + "\r\n"
        + "x-disallow-duplicates: 0\r\n"
        + "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
        + "Content-Length: " + String(totalBody) + "\r\n"
        + "Connection: close\r\n\r\n";

    WiFiClientSecure client;
    client.setInsecure();
    if (!client.connect(EI_INGESTION_HOST, 443)) {
        Serial.println("[UPLOAD] Gagal connect TLS");
        return false;
    }

    // Kirim HTTP header
    client.print(reqHeader);
    // Kirim multipart header
    client.print(partHeader);
    // Kirim WAV header (44 byte, stack)
    uint8_t wavHdr[44];
    _buildWavHeader(wavHdr, numSamples);
    client.write(wavHdr, 44);
    // Stream PCM dari PSRAM → chunk 4KB internal SRAM
    const size_t CHUNK = 4096;
    uint8_t* chunk = (uint8_t*)malloc(CHUNK);
    if (!chunk) { client.stop(); return false; }
    for (size_t sent = 0; sent < pcmBytes; sent += CHUNK) {
        size_t toSend = min(CHUNK, pcmBytes - sent);
        memcpy(chunk, (uint8_t*)pcmBuffer + sent, toSend);
        client.write(chunk, toSend);
    }
    free(chunk);
    // Kirim multipart footer
    client.print(partFooter);

    // Baca response
    uint32_t t = millis();
    while (!client.available() && millis() - t < 10000) delay(10);
    String statusLine = client.readStringUntil('\n');
    // Parse HTTP status code dari "HTTP/1.1 200 OK"
    int code = statusLine.substring(9, 12).toInt();
    client.stop();

    if (code == 200 || code == 201) return true;
    Serial.printf("[UPLOAD] HTTP %d: %s\n", code, statusLine.c_str());
    return false;
}

// ----------------------------------------------------------------
// Private — LED
// ----------------------------------------------------------------
void SampleGetter::_ledOn()  { digitalWrite(PIN_LED, LOW);  }
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
    Serial.printf( "| Duration record : %-4.0f ms     │\n", _durationTime);
    Serial.printf( "│ Upload OK   : %-4d sampel     │\n", _sampleCount);
    Serial.printf( "│ WiFi        : %-14s │\n",
        WiFi.status() == WL_CONNECTED ? "Tersambung" : "Terputus");
    Serial.println("├──────────────────────────────┤");
    Serial.println("│ [Tekan singkat] → Rekam      │");
    Serial.println("│ [Tahan 2 detik] → Ganti label│");
    Serial.println("└──────────────────────────────┘");
}
