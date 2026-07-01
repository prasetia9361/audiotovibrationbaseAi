#pragma once

// ================================================================
// CONFIG.H — Konfigurasi global proyek Audio-to-Vibration AI
//
// Diletakkan di lib/AppConfig/ agar PlatformIO otomatis
// menambahkan foldernya ke include path semua library dan src/.
//
// Build flags (di-set otomatis oleh platformio.ini):
//   GENERIC_ESP32S3  → pin mapping ESP32-S3 DevKit
//   SAMPLER_MODE     → aktifkan konfigurasi pengambilan data
// ================================================================

// ----------------------------------------------------------------
// PIN — INMP441 I2S Microphone
// ----------------------------------------------------------------
#ifdef GENERIC_ESP32S3
    #define PIN_I2S_SCK     GPIO_NUM_2    // I2S_MIC_SERIAL_CLOCK
    #define PIN_I2S_WS      GPIO_NUM_3    // I2S_MIC_LEFT_RIGHT_CLOCK
    #define PIN_I2S_SD      GPIO_NUM_4    // I2S_MIC_SERIAL_DATA
#else
    // Seeed XIAO ESP32-S3 Plus
    #define PIN_I2S_SCK     D8
    #define PIN_I2S_WS      D9
    #define PIN_I2S_SD      D10

    // ----------------------------------------------------------------
    // PIN — LR7843 MOSFET → Motor Vibrasi 3V  (env aplikasi utama)
    // ----------------------------------------------------------------

    #define PIN_MOTOR       D0
#endif



// ----------------------------------------------------------------
// AUDIO — Pengaturan I2S dan buffer
// ----------------------------------------------------------------
#define AUDIO_SAMPLE_RATE       16000U
#define AUDIO_BUFFER_MS         1000U
#define AUDIO_BUFFER_SIZE       (AUDIO_SAMPLE_RATE * AUDIO_BUFFER_MS / 1000U)
#define AUDIO_DMA_BUF_COUNT     8
#define AUDIO_DMA_BUF_LEN       512
#define SAMPLER_MAX_DURATION_MS  120000U  // maks 120 detik (~3.8MB @16kHz int16, aman untuk PSRAM 8MB)

// ----------------------------------------------------------------
// MOTOR PWM
// ----------------------------------------------------------------
#define MOTOR_PWM_FREQ          1000
#define MOTOR_PWM_RESOLUTION    8        // bit (duty 0–255)

// ----------------------------------------------------------------
// AI CLASSIFIER
// ----------------------------------------------------------------
#define CONFIDENCE_THRESHOLD    0.75f

// Jumlah frame berturut-turut dengan label sama (di atas threshold) yang
// diperlukan sebelum memicu vibrasi. Mengurangi false positive dari satu
// frame yang kebetulan lolos threshold. Set 1 untuk menonaktifkan.
#define DETECTION_CONSECUTIVE_HITS  2

#define LABEL_NOISE             0
#define LABEL_KLAKSON           1
#define LABEL_SIRINE            2

// ----------------------------------------------------------------
// SERIAL
// ----------------------------------------------------------------
#define SERIAL_BAUD_RATE        115200

// ================================================================
// SAMPLER MODE — hanya aktif saat build dengan flag -DSAMPLER_MODE
// ================================================================
#ifdef SAMPLER_MODE

// --- PIN Button ---
#ifdef GENERIC_ESP32S3
    #define PIN_BUTTON      GPIO_NUM_13
    #define PIN_LED         GPIO_NUM_18
#else
    #define PIN_BUTTON      D3
    #define PIN_LED         LED_BUILTIN
#endif

// --- Button timing ---
#define BTN_DEBOUNCE_MS         50
#define BTN_LONG_PRESS_MS       2000

// --- Kredensial rahasia (WIFI_SSID, WIFI_PASSWORD, EI_API_KEY) ---
// Didefinisikan di secrets.h (di-ignore git). Salin secrets.h.example
// menjadi secrets.h lalu isi nilai asli sebelum build SAMPLER_MODE.
#include "secrets.h"

// --- WiFi (non-rahasia) ---
#define WIFI_TIMEOUT_MS         15000

// --- Edge Impulse Ingestion API (non-rahasia) ---
#define EI_INGESTION_HOST       "ingestion.edgeimpulse.com"
#define EI_INGESTION_URL        "/api/training/files"

// --- Label kelas ---
#define LABEL_COUNT             3
static const char* const SAMPLE_LABELS[LABEL_COUNT] = {
    "noise",    // 0
    "klakson",  // 1
    "sirine"    // 2
};

#endif // SAMPLER_MODE
