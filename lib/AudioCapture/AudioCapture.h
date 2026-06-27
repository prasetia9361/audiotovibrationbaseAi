#pragma once

#include <Arduino.h>
#include <driver/i2s_std.h>
#include "../AppConfig/config.h"

// ================================================================
// AudioCapture — Membaca audio dari mikrofon INMP441 via I2S
// Menggunakan I2S API baru (i2s_std.h) — ESP32 Arduino Core v3.x
// ================================================================
class AudioCapture {
public:
    AudioCapture();

    // Inisialisasi driver I2S. Kembalikan true jika berhasil.
    bool begin();

    // Ambil satu window audio (durasi = AUDIO_BUFFER_MS).
    // Blokir hingga buffer penuh. Kembalikan true jika berhasil.
    bool capture();

    // Akses buffer hasil capture (int16, mono, AUDIO_SAMPLE_RATE Hz).
    int16_t* getBuffer();
    size_t   getBufferSize() const;

    bool isReady() const;

private:
    int16_t          _buffer[AUDIO_BUFFER_SIZE];   // 16.000 samples = 1 detik @16kHz
    i2s_chan_handle_t _rxChan;
    bool             _initialized;
};
