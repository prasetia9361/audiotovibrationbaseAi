#pragma once

#include <Arduino.h>
#include "../AppConfig/config.h"

// ================================================================
// VibrationMotor — Mengontrol motor vibrasi 3V via MOSFET LR7843
// Menggunakan PWM (LEDC) ESP32 Arduino Core v3.x
// ================================================================
class VibrationMotor {
public:
    VibrationMotor();

    // Inisialisasi PWM. Kembalikan true jika berhasil.
    bool begin();

    // Jalankan test singkat saat startup untuk verifikasi hardware.
    void testStartup();

    // Pola vibrasi untuk klakson: 2 denyut pendek cepat.
    void vibrateKlakson();

    // Pola vibrasi untuk sirine: 3 denyut panjang dengan intensitas meningkat.
    void vibrateSirine();

    // Matikan motor.
    void stop();

    bool isReady() const;

private:
    bool _initialized;

    // Tulis nilai duty PWM langsung ke pin motor.
    void write(uint8_t duty);
};
