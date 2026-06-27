#include <Arduino.h>

// ================================================================
// main.cpp — Entry point
//
// Flag build (di-set otomatis oleh platformio.ini):
//   (tidak ada flag)  → jalankan MainApp  (deteksi suara + vibrasi)
//   -DSAMPLER_MODE    → jalankan SampleGetter (ambil data training)
// ================================================================

#ifdef SAMPLER_MODE
    #include "SampleGetter/SampleGetter.h"
    static SampleGetter app;
#else
    #include "MainApp/MainApp.h"
    static MainApp app;
#endif

void setup() {
    app.begin();
}

void loop() {
    app.task();
}
