#include "MainApp.h"

// ================================================================
// MainApp — Implementasi
// ================================================================

MainApp::MainApp()
    : _systemReady(false)
    , _lastLabel(SoundLabel::UNKNOWN)
    , _consecutiveHits(0)
{}

bool MainApp::begin() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(800);
    _printBanner();

    bool ok = true;

    // 1. Inisialisasi motor
    if (!_motor.begin()) {
        Serial.println("[MainApp] ERROR: Motor gagal init");
        ok = false;
    } else {
        _motor.testStartup();
    }

    // 2. Inisialisasi mikrofon
    if (!_audio.begin()) {
        Serial.println("[MainApp] ERROR: AudioCapture gagal init");
        ok = false;
    }

    // 3. Inisialisasi classifier AI
    if (!_classifier.begin()) {
        Serial.println("[MainApp] ERROR: SoundClassifier gagal init");
        ok = false;
    }

    _systemReady = ok;
    if (_systemReady) {
        Serial.println("[MainApp] Semua komponen siap — mulai mendengarkan...\n");
    } else {
        Serial.println("[MainApp] Sistem tidak siap, periksa koneksi hardware.");
    }
    return _systemReady;
}

void MainApp::task() {
    if (!_systemReady) {
        delay(1000);
        return;
    }

    // 1. Capture audio 1 detik
    if (!_audio.capture()) {
        Serial.println("[MainApp] Gagal baca audio, coba lagi...");
        delay(100);
        return;
    }

    // 2. Klasifikasi
    ClassificationResult result = _classifier.classify(
        _audio.getBuffer(),
        _audio.getBufferSize()
    );

    // 3. Respons berdasarkan hasil
    _handleResult(result);
}

// ----------------------------------------------------------------
// Private
// ----------------------------------------------------------------

void MainApp::_handleResult(const ClassificationResult& result) {
    // Abaikan jika confidence terlalu rendah atau label tidak dikenal.
    // NOISE/UNKNOWN/low-confidence me-reset hitungan debounce agar deteksi
    // hanya dipicu oleh frame relevan yang benar-benar berturut-turut.
    if (result.label == SoundLabel::UNKNOWN ||
        result.label == SoundLabel::NOISE   ||
        result.confidence < CONFIDENCE_THRESHOLD) {
        _lastLabel       = SoundLabel::UNKNOWN;
        _consecutiveHits = 0;
        return;
    }

    // Hitung frame berturut-turut dengan label sama
    if (result.label == _lastLabel) {
        _consecutiveHits++;
    } else {
        _lastLabel       = result.label;
        _consecutiveHits = 1;
    }

    // Belum cukup konsisten → tunggu frame berikutnya
    if (_consecutiveHits < DETECTION_CONSECUTIVE_HITS) return;

    // Sudah terpicu → reset agar tidak spam getar tiap frame berikutnya
    _consecutiveHits = 0;

    // Log ke Serial
    Serial.printf("[MainApp] Terdeteksi: %s (%.0f%%)\n",
        result.labelName(),
        result.confidence * 100.0f
    );

    // Aktifkan pola vibrasi yang sesuai
    switch (result.label) {
        case SoundLabel::KLAKSON:
            _motor.vibrateKlakson();
            break;
        case SoundLabel::SIRINE:
            _motor.vibrateSirine();
            break;
        default:
            break;
    }
}

void MainApp::_printBanner() {
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║   Audio-to-Vibration AI              ║");
    Serial.println("║   Alat Bantu Tuna Rungu              ║");
    Serial.println("║   XIAO ESP32-S3 + INMP441            ║");
    Serial.println("╚══════════════════════════════════════╝");
}
