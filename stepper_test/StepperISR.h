/*
 * StepperISR.h
 *
 * Interrupt-timed stepper driver for 28BYJ-48 + ULN2003 on ESP32,
 * driven by a hardware timer ISR (not polling in loop()).
 *
 * DO NOT MODIFY THIS FILE. This is the interface your StepperISR
 * class must implement — write your implementation in StepperISR.cpp.
 *
 * Supports both HALF_STEP (8-step sequence, 4096 steps/rev) and
 * FULL_STEP (4-step sequence, 2048 steps/rev) modes, switchable at
 * runtime via setStepMode() while the motor is idle.
 *
 * NOTE ON UNITS: position (currentPosition/moveTo/distanceToGo) is
 * expressed in raw step counts. A "step" does NOT mean the same
 * physical angle in HALF_STEP vs FULL_STEP mode — switching modes
 * changes what a given position value means physically. This is left
 * as-is deliberately; see the lab reflection questions.
 *
 * Written for Arduino-ESP32 core 3.x timer API:
 *   timerBegin(freq_hz), timerAttachInterrupt(timer, callback),
 *   timerAlarm(timer, ticks, autoreload, count)
 */

#ifndef STEPPER_ISR_H
#define STEPPER_ISR_H

#include <Arduino.h>

enum StepMode {
  HALF_STEP,   // 8-step sequence, 4096 steps/revolution
  FULL_STEP    // 4-step sequence, 2048 steps/revolution
};

class StepperISR {
  public:
    StepperISR();

    // Configure pins + hardware timer. Call once from setup().
    // Defaults to HALF_STEP mode.
    void begin(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4);

    // Set the step rate (steps/sec) used by moveTo()/move(), since
    // those calls take no speed argument of their own. Takes effect
    // immediately on the hardware timer. There is no acceleration in
    // this library, so this is the actual instantaneous speed the
    // motor steps at — not a ceiling ramped toward, hence "setSpeed"
    // rather than "setMaxSpeed".
    // If given a negative value, the absolute value is used instead
    // and a warning is printed to Serial (speed here is a magnitude —
    // direction in position mode comes from distanceToGo()'s sign).
    // NOTE: runSpeed() always overrides this with its own argument,
    // so calling setSpeed() right before runSpeed() has no lasting
    // effect — the runSpeed() value wins.
    void setSpeed(float stepsPerSec);

    // Switch between HALF_STEP and FULL_STEP. Only allowed while the
    // motor is idle (isRunning() == false); returns false and leaves
    // the mode unchanged if the motor is currently running.
    bool setStepMode(StepMode mode);
    StepMode getStepMode();

    // Position mode: go to an absolute step count. Calling this while
    // runSpeed() is active is allowed and immediately interrupts the
    // jog, switching to position mode toward the new target.
    void moveTo(int32_t absolute);

    // Position mode: go relative to current position.
    void move(int32_t relative);

    // Constant-speed mode: step forever in one direction until stop().
    // Positive = one direction, negative = the other. Calling this
    // while moveTo()/move() is active is allowed and immediately
    // interrupts the move, switching to constant-speed mode. While in
    // speed mode, distanceToGo() reads 0 (there is no target).
    void runSpeed(float stepsPerSec);

    // Immediately stop and de-energize all coils.
    void stop();

    bool isRunning();
    int32_t distanceToGo();
    int32_t currentPosition();

    // Call every loop() iteration. Consumes the pending-step flag set
    // by the ISR and advances exactly one step if it's due.
    void run();

  private:
    uint8_t _pins[4];
    hw_timer_t *_timer;

    // Only _stepFlag is touched inside the ISR — that's the only
    // member that needs to be volatile.
    volatile bool _stepFlag;

    int32_t _currentPos;
    int32_t _targetPos;
    int8_t  _seqIndex;
    int8_t  _moveDir;
    float   _speed;
    bool    _running;
    bool    _speedMode;

    StepMode _mode;
    //this is a pointer to a an array of 4 bytes
    const uint8_t (*_seqTable)[4];  // points at HALF_STEP or FULL_STEP
    uint8_t _seqLen;                // 8 or 4

    void applyCoils();
    void deenergize();
    void setIntervalUs(uint32_t intervalUs);

    // Static-callback pattern: timerAttachInterrupt needs a plain
    // function pointer (no captures), so we keep a static instance
    // pointer and route the interrupt to it via onTimerISR(). This
    // supports ONE stepper instance.
    static StepperISR *_instance;
    static void IRAM_ATTR onTimerISR();
};

#endif
