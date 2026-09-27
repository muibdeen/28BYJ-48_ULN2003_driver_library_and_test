/*
 * StepperISR_test.ino
 *
 * Uses the StepperISR library (StepperISR.h / StepperISR.cpp) to
 * drive a 28BYJ-48 via ULN2003 on ESP32, controlled over Serial.
 *
 * Commands: remiember to put a space before the the value
 *   moveTo <n>      - go to absolute step count
 *   move <n>        - go relative to current position
 *   speed <n>       - set speed (steps/sec)
 *   runSpeed <n>    - constant-speed mode (+/- direction), run until stop
 *   stop            - stop and de-energize
 *   mode half|full  - switch step mode (only while stopped)
 *   status          - print current state
 *
 * Place StepperISR.h and StepperISR.cpp in the same sketch folder
 * as this file (Arduino IDE will compile them together automatically).
 */

#include "StepperISR.h"

// TODO: set your pins
const uint8_t IN1 = 16;
const uint8_t IN2 = 17;
const uint8_t IN3 = 21;
const uint8_t IN4 = 22;
const uint8_t LED_PIN = 2;

StepperISR stepper;

// ---------- Serial command parsing ----------
void handleSerial() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  int spaceIdx = line.indexOf(' ');
  String cmd    = (spaceIdx == -1) ? line : line.substring(0, spaceIdx);
  String argStr = (spaceIdx == -1) ? ""   : line.substring(spaceIdx + 1);
  cmd.toLowerCase();
  argStr.trim();
  argStr.toLowerCase();

  if (cmd == "moveto") {
    int32_t v = argStr.toInt();
    stepper.moveTo(v);
    Serial.printf("moveTo %ld\n", (long)v);

  } else if (cmd == "move") {
    int32_t v = argStr.toInt();
    stepper.move(v);
    Serial.printf("move %ld\n", (long)v);

  } else if (cmd == "speed") {
    float v = argStr.toFloat();
    stepper.setSpeed(v);
    Serial.printf("Speed set to %.1f steps/s\n", v);

  } else if (cmd == "runspeed") {
    float v = argStr.toFloat();
    stepper.runSpeed(v);
    Serial.printf("runSpeed %.1f steps/s\n", v);

  } else if (cmd == "stop") {
    stepper.stop();
    Serial.println("stopped");

  } else if (cmd == "mode") {
    bool ok;
    if (argStr == "half") {
      ok = stepper.setStepMode(HALF_STEP);
      Serial.println(ok ? "mode -> HALF_STEP" : "mode change rejected (motor running, call stop() first)");
    } else if (argStr == "full") {
      ok = stepper.setStepMode(FULL_STEP);
      Serial.println(ok ? "mode -> FULL_STEP" : "mode change rejected (motor running, call stop() first)");
    } else {
      Serial.println("usage: mode half | mode full");
    }

  } else if (cmd == "status") {
    Serial.printf("pos=%ld target=%ld running=%d distanceToGo=%ld mode=%s\n",
                  (long)stepper.currentPosition(),
                  (long)(stepper.currentPosition() + stepper.distanceToGo()),
                  stepper.isRunning(),
                  (long)stepper.distanceToGo(),
                  (stepper.getStepMode() == HALF_STEP) ? "HALF_STEP" : "FULL_STEP");

  } else {
    Serial.println("unknown cmd. try: moveTo <n> | move <n> | speed <n> | runSpeed <n> | stop | mode half|full | status");
  }
}

// ---------- Non-blocking proof ----------
uint32_t lastBlinkMs = 0;
bool     ledState    = false;
uint32_t lastPrintMs = 0;

void nonBlockingProof() {
  uint32_t now = millis();

  if (now - lastBlinkMs >= 500) {
    lastBlinkMs = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  if (now - lastPrintMs >= 1000) {
    lastPrintMs = now;
    Serial.printf("[t=%lu ms] pos=%ld running=%d\n",
                  (unsigned long)now, (long)stepper.currentPosition(), stepper.isRunning());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  stepper.begin(IN1, IN2, IN3, IN4); // defaults to HALF_STEP
  stepper.setSpeed(500);

  Serial.println("StepperISR example ready.");
  Serial.println("Commands: moveTo <n> | move <n> | speed <n> | runSpeed <n> | stop | mode half|full | status");
}

void loop() {
  handleSerial();
  stepper.run();
  nonBlockingProof();
}
