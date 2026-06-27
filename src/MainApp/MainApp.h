#pragma once

#include <Arduino.h>
#include "AudioCapture.h"
#include "VibrationMotor.h"
#include "SoundClassifier.h"
#include "config.h"

// ================================================================
// MainApp — Orkestrasi utama aplikasi Audio-to-Vibration AI
//
// Alur kerja:
//   begin() → inisialisasi semua komponen
//   task()  → dipanggil terus di loop():
//             capture audio → classify → trigger motor
// ================================================================
class MainApp {
public:
    MainApp();

    // Inisialisasi semua komponen. Panggil sekali di setup().
    bool begin();

    // Satu siklus: capture → classify → respond. Panggil di loop().
    void task();

private:
    AudioCapture    _audio;
    VibrationMotor  _motor;
    SoundClassifier _classifier;

    bool _systemReady;

    // Respons terhadap hasil klasifikasi
    void _handleResult(const ClassificationResult& result);

    // Print banner saat startup
    void _printBanner();
};
