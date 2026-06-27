#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "AudioCapture.h"
#include "config.h"

// ================================================================
// SampleGetter — Ambil sampel audio dan kirim ke Edge Impulse
//
// Cara pakai:
//   - Tekan tombol SEKALI (singkat) → rekam & upload label aktif
//   - Tahan tombol 2 detik         → ganti ke label berikutnya menngunakan input serial 
//
// Alur per tekan:
//   [Button] → LED nyala → rekam 1 detik → LED kedip → upload WiFi
//            → LED mati (sukses) / LED cepat (gagal)
// ================================================================
class SampleGetter {
public:
    SampleGetter();

    // Inisialisasi hardware dan WiFi. Panggil di setup().
    bool begin();

    // Polling tombol dan handle event. Panggil di loop().
    void task();

private:
    AudioCapture  _audio;
    WiFiClientSecure _wifiClient;

    uint8_t  _labelIndex;       // Index label aktif
    uint32_t _sampleCount;      // Jumlah sampel berhasil diupload sesi ini
    bool     _wifiConnected;

    // ---- Button state machine ----
    bool     _btnLastState;
    bool     _inputState;
    bool     _isInputagain;
    uint32_t _btnPressTime;
    bool     _longPressHandled;

    // ---- Operasi utama ----
    void _recordAndUpload();
    void _nextLabel();

    // ---- WiFi ----
    bool _connectWiFi();

    // ---- WAV builder ----
    // Buat WAV header 44 byte dan tulis ke dst.
    void _buildWavHeader(uint8_t* dst, uint32_t numSamples);

    // ---- Upload ke Edge Impulse Ingestion API ----
    // Kembalikan true jika HTTP 200.
    bool _uploadToEdgeImpulse(int16_t* pcmBuffer, size_t numSamples);

    // ---- LED feedback ----
    void _ledOn();
    void _ledOff();
    void _ledBlink(uint8_t times, uint16_t periodMs);

    // ---- Util ----
    void _printStatus();
};
