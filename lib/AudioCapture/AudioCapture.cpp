#include "AudioCapture.h"

// Ukuran chunk untuk baca per iterasi (int32 → convert ke int16)
// 256 × 4 bytes = 1KB stack — aman untuk ESP32
static const int READ_CHUNK = 256;

AudioCapture::AudioCapture() : _rxChan(nullptr), _initialized(false) {
    memset(_buffer, 0, sizeof(_buffer));
}

bool AudioCapture::begin() {
    // --- 1. Buat channel I2S RX ---
    i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chanCfg, NULL, &_rxChan);
    if (err != ESP_OK) {
        Serial.printf("[AudioCapture] Gagal buat channel: 0x%x\n", err);
        return false;
    }

    // --- 2. Konfigurasi mode I2S Standard (Philips) ---
    // INMP441 output: 24-bit data dalam frame 32-bit, left channel (L/R=GND)
    i2s_std_config_t stdCfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                        I2S_DATA_BIT_WIDTH_32BIT,   // INMP441 pakai frame 32-bit
                        I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)PIN_I2S_SCK,
            .ws   = (gpio_num_t)PIN_I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)PIN_I2S_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    err = i2s_channel_init_std_mode(_rxChan, &stdCfg);
    if (err != ESP_OK) {
        Serial.printf("[AudioCapture] Gagal init std mode: 0x%x\n", err);
        i2s_del_channel(_rxChan);
        return false;
    }

    // --- 3. Aktifkan channel ---
    err = i2s_channel_enable(_rxChan);
    if (err != ESP_OK) {
        Serial.printf("[AudioCapture] Gagal enable channel: 0x%x\n", err);
        i2s_del_channel(_rxChan);
        return false;
    }

    _initialized = true;
    Serial.println("[AudioCapture] INMP441 siap (i2s_std)");
    return true;
}

bool AudioCapture::capture() {
    if (!_initialized) return false;

    // Baca dalam chunk 256 sample (int32) untuk menghindari alokasi stack besar.
    // INMP441 kirim 32-bit per sample → shift kanan 14 bit → int16 yang bermakna.
    int32_t chunk[READ_CHUNK];
    int     offset = 0;

    while (offset < (int)AUDIO_BUFFER_SIZE) {
        int     toRead   = min(READ_CHUNK, (int)AUDIO_BUFFER_SIZE - offset);
        size_t  bytesRead = 0;

        esp_err_t err = i2s_channel_read(
            _rxChan,
            chunk,
            toRead * sizeof(int32_t),
            &bytesRead,
            pdMS_TO_TICKS(1000)
        );

        if (err != ESP_OK || bytesRead == 0) {
            Serial.printf("[AudioCapture] Error baca: 0x%x\n", err);
            return false;
        }

        int samplesRead = (int)(bytesRead / sizeof(int32_t));
        for (int i = 0; i < samplesRead; i++) {
            // Ambil 18 bit teratas dari frame 32-bit INMP441
            _buffer[offset + i] = (int16_t)(chunk[i] >> 14);
        }
        offset += samplesRead;
    }
    return true;
}

int16_t* AudioCapture::getBuffer() {
    return _buffer;
}

size_t AudioCapture::getBufferSize() const {
    return AUDIO_BUFFER_SIZE;
}

bool AudioCapture::isReady() const {
    return _initialized;
}
