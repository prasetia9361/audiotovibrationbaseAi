#include "SoundClassifier.h"

// ================================================================
// SoundClassifier — Implementasi
// ================================================================

SoundClassifier::SoundClassifier()
    : _modelReady(false)
#ifndef EDGE_IMPULSE_ENABLED
    , _demoStep(0)
#endif
{}

bool SoundClassifier::begin() {
#ifdef EDGE_IMPULSE_ENABLED
    // --- Mode Edge Impulse ---
    // Model sudah tertanam di library, tidak perlu inisialisasi khusus.
    // Cukup validasi bahwa ukuran buffer sesuai dengan yang diharapkan model.
    if (AUDIO_BUFFER_SIZE != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.printf(
            "[SoundClassifier] PERINGATAN: ukuran buffer (%d) tidak sesuai "
            "dengan ekspektasi model Edge Impulse (%d)\n",
            (int)AUDIO_BUFFER_SIZE,
            (int)EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE
        );
        return false;
    }
    _modelReady = true;
    Serial.println("[SoundClassifier] Model Edge Impulse siap");
#else
    // --- Mode Demo (tanpa Edge Impulse) ---
    _modelReady = true;
    Serial.println("[SoundClassifier] Mode DEMO aktif (Edge Impulse belum ditambahkan)");
    Serial.println("[SoundClassifier] Untuk mengaktifkan AI: uncomment #define EDGE_IMPULSE_ENABLED");
#endif
    return true;
}

ClassificationResult SoundClassifier::classify(int16_t* buffer, size_t size) {
    if (!_modelReady) {
        return { SoundLabel::UNKNOWN, 0.0f };
    }

#ifdef EDGE_IMPULSE_ENABLED
    return _runInference(buffer, size);
#else
    (void)buffer;
    (void)size;
    return _runDemo();
#endif
}

bool SoundClassifier::isReady() const {
    return _modelReady;
}

// ----------------------------------------------------------------
// Private — Mode Edge Impulse
// ----------------------------------------------------------------
#ifdef EDGE_IMPULSE_ENABLED
ClassificationResult SoundClassifier::_runInference(int16_t* buffer, size_t size) {
    // Bungkus buffer int16 ke format signal Edge Impulse
    signal_t signal;
    int err = numpy::signal_from_buffer(buffer, size, &signal);
    if (err != 0) {
        Serial.printf("[SoundClassifier] Error signal: %d\n", err);
        return { SoundLabel::UNKNOWN, 0.0f };
    }

    // Jalankan classifier
    ei_impulse_result_t result = { 0 };
    EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.printf("[SoundClassifier] Error inferensi: %d\n", res);
        return { SoundLabel::UNKNOWN, 0.0f };
    }

    // Tampilkan semua skor ke Serial
    Serial.println("--- Hasil Deteksi ---");
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        Serial.printf("  %-10s: %.1f%%\n",
            result.classification[i].label,
            result.classification[i].value * 100.0f
        );
    }

    // Cari label dengan confidence tertinggi
    float      maxVal = 0.0f;
    SoundLabel maxLabel = SoundLabel::UNKNOWN;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > maxVal) {
            maxVal   = result.classification[i].value;
            maxLabel = static_cast<SoundLabel>(i);
        }
    }

    return { maxLabel, maxVal };
}
#endif

// ----------------------------------------------------------------
// Private — Mode Demo (simulasi urutan deteksi untuk test hardware)
// ----------------------------------------------------------------
#ifndef EDGE_IMPULSE_ENABLED
ClassificationResult SoundClassifier::_runDemo() {
    ClassificationResult res;
    switch (_demoStep % 3) {
        case 0: res = { SoundLabel::NOISE,   0.97f }; break;
        case 1: res = { SoundLabel::KLAKSON, 0.93f }; break;
        case 2: res = { SoundLabel::SIRINE,  0.88f }; break;
    }
    _demoStep++;
    return res;
}
#endif
