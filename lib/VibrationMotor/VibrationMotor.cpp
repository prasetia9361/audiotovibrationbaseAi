#include "VibrationMotor.h"

VibrationMotor::VibrationMotor() : _initialized(false) {}

bool VibrationMotor::begin() {
    // ESP32 Arduino Core v3.x: ledcAttach menggantikan ledcSetup + ledcAttachPin
    bool ok = ledcAttach(PIN_MOTOR, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    if (!ok) {
        Serial.println("[VibrationMotor] Gagal inisialisasi PWM");
        return false;
    }
    write(0);
    _initialized = true;
    Serial.println("[VibrationMotor] Siap");
    return true;
}

void VibrationMotor::testStartup() {
    if (!_initialized) return;
    Serial.println("[VibrationMotor] Test startup...");
    write(180);
    delay(350);
    write(0);
    Serial.println("[VibrationMotor] OK");
}

void VibrationMotor::vibrateKlakson() {
    if (!_initialized) return;
    // Pola: 2 denyut pendek cepat
    for (int i = 0; i < 2; i++) {
        write(200);
        delay(120);
        write(0);
        delay(80);
    }
}

void VibrationMotor::vibrateSirine() {
    if (!_initialized) return;
    // Pola: 3 denyut panjang, intensitas bertahap meningkat
    for (int i = 0; i < 3; i++) {
        write(150 + (i * 35));   // 150 → 185 → 220
        delay(280);
        write(0);
        delay(120);
    }
}

void VibrationMotor::stop() {
    write(0);
}

bool VibrationMotor::isReady() const {
    return _initialized;
}

// --- Private ---

void VibrationMotor::write(uint8_t duty) {
    ledcWrite(PIN_MOTOR, duty);
}
