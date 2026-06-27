#pragma once

#include <Arduino.h>
#include "../AppConfig/config.h"

// ================================================================
// Uncomment baris berikut setelah library Edge Impulse ditambahkan
// ke folder /lib. Ganti nama sesuai nama project Edge Impulse kamu.
// ================================================================
// #define EDGE_IMPULSE_ENABLED
// #include <klakson-sirine-detector_inferencing.h>

// ----------------------------------------------------------------
// Enum hasil klasifikasi
// ----------------------------------------------------------------
enum class SoundLabel : int8_t {
    NOISE   = LABEL_NOISE,
    KLAKSON = LABEL_KLAKSON,
    SIRINE  = LABEL_SIRINE,
    UNKNOWN = -1
};

// ----------------------------------------------------------------
// Struct yang membawa label dan tingkat keyakinan AI
// ----------------------------------------------------------------
struct ClassificationResult {
    SoundLabel label;
    float      confidence;   // 0.0 – 1.0

    // Helper: kembalikan nama label sebagai string
    const char* labelName() const {
        switch (label) {
            case SoundLabel::KLAKSON: return "KLAKSON";
            case SoundLabel::SIRINE:  return "SIRINE";
            case SoundLabel::NOISE:   return "noise";
            default:                  return "unknown";
        }
    }
};

// ================================================================
// SoundClassifier — Menjalankan inferensi AI pada buffer audio
// ================================================================
class SoundClassifier {
public:
    SoundClassifier();

    // Inisialisasi model. Kembalikan true jika berhasil.
    bool begin();

    // Jalankan inferensi pada buffer audio int16.
    // buffer   : pointer ke array int16_t hasil AudioCapture
    // size     : jumlah sample (bukan bytes)
    // return   : ClassificationResult dengan label dan confidence tertinggi
    ClassificationResult classify(int16_t* buffer, size_t size);

    bool isReady() const;

private:
    bool _modelReady;

#ifdef EDGE_IMPULSE_ENABLED
    ClassificationResult _runInference(int16_t* buffer, size_t size);
#else
    // Mode demo: dipakai saat Edge Impulse belum ditambahkan
    ClassificationResult _runDemo();
    uint8_t _demoStep;
#endif
};
