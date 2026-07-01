#include "SoundClassifier.h"

// ================================================================
// SoundClassifier — Implementasi
// ================================================================
#ifdef EDGE_IMPULSE_ENABLED
#include <klakson-sirine-detector_inferencing.h>
#endif

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

// Callback untuk konversi int16 → float yang dibutuhkan Edge Impulse SDK
static int16_t* _ei_pcm_buf = nullptr;
static int ei_get_audio_signal_data(size_t offset, size_t length, float* out_ptr) {
    for (size_t i = 0; i < length; i++) {
        out_ptr[i] = (float)_ei_pcm_buf[offset + i] / 32768.0f;
    }
    return EIDSP_OK;
}

// Pemetaan nama label Edge Impulse → SoundLabel.
// PENTING: dipetakan berdasarkan STRING, bukan indeks, karena urutan
// label pada model Edge Impulse belum tentu sama dengan urutan enum
// (mis. model bisa mengurutkan "klakson", "noise", "sirine").
static SoundLabel _labelFromName(const char* name) {
    if (strcmp(name, "klakson") == 0) return SoundLabel::KLAKSON;
    if (strcmp(name, "sirine")  == 0) return SoundLabel::SIRINE;
    if (strcmp(name, "noise")   == 0) return SoundLabel::NOISE;
    return SoundLabel::UNKNOWN;
}

ClassificationResult SoundClassifier::_runInference(int16_t* buffer, size_t size) {
    // Set buffer untuk callback, bungkus ke format signal Edge Impulse
    _ei_pcm_buf = buffer;
    signal_t signal;
    signal.total_length = size;
    signal.get_data     = &ei_get_audio_signal_data;

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

    // Cari label dengan confidence tertinggi.
    // Petakan berdasarkan nama label (bukan indeks) agar aman terhadap
    // perubahan urutan label saat model di-retrain.
    float       maxVal  = 0.0f;
    const char* maxName = nullptr;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > maxVal) {
            maxVal  = result.classification[i].value;
            maxName = result.classification[i].label;
        }
    }

    SoundLabel maxLabel = maxName ? _labelFromName(maxName) : SoundLabel::UNKNOWN;
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
