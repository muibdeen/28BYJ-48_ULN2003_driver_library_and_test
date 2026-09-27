/*
 * StepperISR.cpp
 *
 * YOUR IMPLEMENTATION GOES HERE. Do not modify StepperISR.h.
 *
 * Every function below has a docstring describing exactly what it
 * must do. Bodies are stubbed with a placeholder so the file compiles
 * as-is (with wrong/no-op behavior) — replace each placeholder with a
 * real implementation.
 *
 * You must also fill in the two coil sequence tables below
 * (HALF_STEP_SEQ, FULL_STEP_SEQ) — they determine which ULN2003
 * inputs are energized at each step in the half-step and full-step
 * cycles. The placeholder values given are NOT correct.
 */

#include "StepperISR.h"

// TODO: define the half-step coil sequence here. Each row should be
// {IN1, IN2, IN3, IN4} for one step of the 8-step half-step cycle.
// The placeholder values below are NOT correct — replace them.
static const uint8_t HALF_STEP_SEQ[8][4] = { //one phase on then two phaes on...etc.
  {1,1,0,0},
  {0,1,0,0}, //full step sequencing starts here and goes every other line 
  {0,1,0,0},
  {0,1,1,0},
  {0,0,1,0},
  {0,0,1,1},
  {0,0,0,1},
  {1,0,0,1}
};

// TODO: define the full-step coil sequence here. Each row should be
// {IN1, IN2, IN3, IN4} for one step of the 4-step full-step cycle.
// The placeholder values below are NOT correct — replace them.

// FOR FULL STEP, 2 coils are always active at a time
static const uint8_t FULL_STEP_SEQ[4][4] = {
  {1,1,0,0}, //in1 
  {0,1,1,0}, //in2 
  {0,0,1,1},//in3 
  {1,0,0,1}//in4 
};

// -----------------------------------------------------------------
// Static instance pointer used to route the interrupt to onTimerISR() (see below).
// -----------------------------------------------------------------
StepperISR *StepperISR::_instance = nullptr;

// -----------------------------------------------------------------
// Constructor
//
// Initialize every private member to a safe default:
//   - _timer to nullptr (not yet created)
//   - _stepFlag to false
//   - _currentPos, _targetPos to 0
//   - _seqIndex to 0
//   - _moveDir to +1 (arbitrary default direction)
//   - _speed to some reasonable default, e.g. 500
//   - _running to false
//   - _speedMode to false
//   - _mode to HALF_STEP
//   - _seqTable to point at HALF_STEP_SEQ
//   - _seqLen to 8
// -----------------------------------------------------------------
StepperISR::StepperISR() {
  _timer = nullptr;

  // TODO: initialize the remaining private members:
  //
  // setting variables via contructor method within curly braces 
  // intead of using initlaizer method ":" before curly braces
  _stepFlag = false;//   - _stepFlag to false
  _currentPos = 0;//   - _currentPos, _targetPos to 0
  _targetPos = 0;
  _seqIndex = 0;///   - _seqIndex to 0
  _moveDir = 1;//- _moveDir to +1 (arbitrary default direction)
  _speed = 500;//   - _speed to some reasonable default, e.g. 500
  _running = false;//   - _running to false
  _speedMode = false;//   - _speedMode to false
  _mode = HALF_STEP;//   - _mode to HALF_STEP
  _seqTable = HALF_STEP_SEQ;//   - _seqTable to point at HALF_STEP_SEQ
  _seqLen = 8; //   - _seqLen to 8
}

// -----------------------------------------------------------------
// onTimerISR()
//
// This is the actual interrupt service routine attached to the timer.
// It must be extremely short — this is the "keep the ISR minimal"
// requirement from the assignment.
//
// timerAttachInterrupt() requires a plain function pointer with no
// captured state, so a non-static member function cannot be used
// directly as the ISR. The pattern used here: keep a static pointer
// (_instance) to the one StepperISR object in use, and have this
// static function route the interrupt to it.
//
// What this function must do:
//   - If _instance is not null, set _instance->_stepFlag = true.
//   - Nothing else. No digitalWrite(), no math, no Serial.
// -----------------------------------------------------------------
void IRAM_ATTR StepperISR::onTimerISR() {
  if (_instance != nullptr) {
    // TODO: set the pending-step flag on the active instance
    //
    // need to use the arrow pointer to access the private variable through the isntance pointer.
    // the pointer operation allows us to modify values while within this static void function
    _instance->_stepFlag = true;
  }
}

// -----------------------------------------------------------------
// begin(in1, in2, in3, in4)
//
// What this function must do:
//   1. Store the four pin numbers (in _pins[0..3]).
//   2. Set all four pins to OUTPUT mode.
//   3. De-energize all coils (see deenergize() below).
//   4. Register this object as the active instance so onTimerISR()
//      knows which object to route the interrupt to — this part is
//      already done for you below (_instance = this).
//   5. Create the hardware timer at 1 MHz (1 tick = 1 microsecond)
//      using timerBegin().
//   6. Attach the ISR with timerAttachInterrupt(), passing
//      &StepperISR::onTimerISR.
//   7. Set a default alarm period with timerAlarm() (autoreload
//      true, reload count 0) — 2000 ticks (500 steps/s) is a
//      reasonable default.
//   8. Call timerStop() so the timer is idle until a move/run is
//      actually commanded.
//   9. Call setSpeed(_speed) to make sure the interval matches
//      whatever _speed was initialized to.
// -----------------------------------------------------------------
void StepperISR::begin(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
  // TODO: steps 1-3 (store pins, set OUTPUT mode, de-energize)
  
  

  // store pins
  _pins[0] = in1;
  _pins[1] = in2;
  _pins[2] = in3;
  _pins[3] = in4;

  //set output mode
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // de-energize
  deenergize();

  _instance = this; // step 4 — done for you

  // TODO: steps 5-9 (create timer, attach ISR, set default alarm,
  // stop timer, sync speed)
  
  //set timer to go at 1MHz, 1 microsecond
  _timer = timerBegin(1000000);

  //attach interrupt to timer var 
  timerAttachInterrupt(_timer, &onTimerISR);

  //set default alarm
  //configures alarm value and autoreload the timer
  //2000 ticks = 500 steps
  //autoreload of true means time runs forever
  timerAlarm(_timer, 2000, true, 0);

  //stop timer
  timerStop(_timer);

  setSpeed(_speed);
}

// -----------------------------------------------------------------
// setIntervalUs(intervalUs)  [private helper]
//
// What this function must do:
//   - If _timer is null, return immediately (not yet begin()'d).
//   - Otherwise, call timerAlarm(_timer, intervalUs, true, 0) to set
//     the timer's alarm period to intervalUs microseconds, with
//     autoreload enabled.
// -----------------------------------------------------------------
void StepperISR::setIntervalUs(uint32_t intervalUs) {
  // TODO: implement
  if (_timer == nullptr ){
    return;
  }
  else{
    timerAlarm(_timer, intervalUs, true, 0);
    return;
  }

}

// -----------------------------------------------------------------
// setSpeed(stepsPerSec)
//
// What this function must do:
//   - If stepsPerSec is negative, print a warning to Serial and use
//     its absolute value instead (speed here is a magnitude —
//     direction in position mode comes from distanceToGo()'s sign,
//     not from this argument).
//   - Guard against a resulting value of 0 (clamp to something small
//     and positive, e.g. 1, to avoid divide-by-zero).
//   - Store the value in _speed.
//   - Convert steps/sec to a microsecond interval and call
//     setIntervalUs() with it.
//
// NOTE: think carefully about the type of the interval calculation.
// See Reflection Question 4 in the assignment about what a plain
// integer cast of (1000000.0 / stepsPerSec) does to accuracy — you
// are not required to "fix" this, just to understand and explain it.
// -----------------------------------------------------------------
void StepperISR::setSpeed(float stepsPerSec) {
  // TODO: implement
  
  // warning against negativ number input
  if (stepsPerSec < 0){
    Serial.print("Negative input value, assigning positive counterpart");
    stepsPerSec = abs(stepsPerSec);
  }

  //guard against a 0 value input
  if (stepsPerSec == 0){
    stepsPerSec = stepsPerSec + 0.0001;
  }

  //store the true steps per sec value to _speed, value ensures that we dont have 0 input
  //and we dont submit a 0 value 
  _speed = stepsPerSec;
}

// -----------------------------------------------------------------
// setStepMode(mode)
//
// What this function must do:
//   - If the motor is currently running (isRunning() == true),
//     do NOT change anything — return false.
//   - If mode is already the current mode, return true (no-op).
//   - Otherwise, switch _seqTable / _seqLen to point at the
//     requested mode's table (HALF_STEP_SEQ, length 8, or
//     FULL_STEP_SEQ, length 4), update _mode, reset _seqIndex to 0,
//     de-energize the coils, and return true.
// -----------------------------------------------------------------
bool StepperISR::setStepMode(StepMode mode) {
  // TODO: implement
  
  // exit if motor is already running
  if (isRunning() == true){
    return false;
  }
  
  //make no changes if current mode is already set to requested mode
  if (getStepMode() == mode){
    return true;
  }
  else { // set mode to requested mode
    if (mode == HALF_STEP ){
      _seqTable = HALF_STEP_SEQ;
      _seqLen = 8;
    }
    else { //others set to full step specs
      _seqTable = FULL_STEP_SEQ;
      _seqLen = 4;
    }
    _mode = mode;
    _seqIndex = 0;
    deenergize();
    return true;
  }
  
  
}

// -----------------------------------------------------------------
// getStepMode()
//
// Return the current _mode.
// -----------------------------------------------------------------
StepMode StepperISR::getStepMode() {
  // TODO: implement
  //
  return _mode; // placeholder
}

// -----------------------------------------------------------------
// moveTo(absolute)
//
// What this function must do:
//   - Set _targetPos to the given absolute position.
//   - Set _speedMode to false (position mode).
//   - Set _running to true.
//   - Call timerStart(_timer) to (re)start the hardware timer.
// -----------------------------------------------------------------
void StepperISR::moveTo(int32_t absolute) {
  // TODO: implement
  //
  _targetPos = absolute;
  _speedMode = false;
  _running = true;
  timerStart(_timer);
}

// -----------------------------------------------------------------
// move(relative)
//
// What this function must do:
//   - Compute the absolute target as _currentPos + relative, and
//     call moveTo() with it.
// -----------------------------------------------------------------
void StepperISR::move(int32_t relative) {
  // TODO: implement
  //
  moveTo(_currentPos + relative);
}

// -----------------------------------------------------------------
// runSpeed(stepsPerSec)
//
// What this function must do:
//   - Set _speedMode to true.
//   - Set _moveDir based on the sign of stepsPerSec (+1 or -1).
//   - Call setSpeed() with the absolute value of stepsPerSec.
//   - Set _targetPos = _currentPos (there's no real target in speed
//     mode — this keeps distanceToGo() reading 0 while jogging).
//   - Set _running to true.
//   - Call timerStart(_timer).
// -----------------------------------------------------------------
void StepperISR::runSpeed(float stepsPerSec) {
  // TODO: implement
  //
  _speedMode = true;

  (stepsPerSec < 0) ? _moveDir = -1 : _moveDir = 1;
  _targetPos = _currentPos;
  _running = true;
  timerStart(_timer);
}

// -----------------------------------------------------------------
// stop()
//
// What this function must do:
//   - Set _running to false.
////   - Call timerStop(_timer).
//   - De-energize all coils.
// -----------------------------------------------------------------
void StepperISR::stop() {
  // TODO: implement
  //
  _running = false;
  timerStop(_timer);
  deenergize();
}

// -----------------------------------------------------------------
// isRunning() / distanceToGo() / currentPosition()
//
// Simple state queries — return the corresponding private member(s).
// distanceToGo() = _targetPos - _currentPos.
// -----------------------------------------------------------------
bool StepperISR::isRunning() {
  // TODO: implement
  return _running; // placeholder
}

int32_t StepperISR::distanceToGo() {
  // TODO: implement
  return _targetPos - _currentPos; // placeholder
}

int32_t StepperISR::currentPosition() {
  // TODO: implement
  return _currentPos; // placeholder
}

// -----------------------------------------------------------------
// applyCoils()  [private helper]
//
// What this function must do:
//   - Write each of the 4 pins (_pins[0..3]) to the value found in
//     _seqTable[_seqIndex][0..3].
// -----------------------------------------------------------------
void StepperISR::applyCoils() {
  // TODO: implement
  //
  for(int i=0; i<=3; i++){
    _pins[i] = _seqTable[_seqIndex][i];
  }
}

// -----------------------------------------------------------------
// deenergize()  [private helper]
//
// What this function must do:
//   - Write all 4 pins LOW.
// -----------------------------------------------------------------
void StepperISR::deenergize() {
  // TODO: implement
  for(int i=0; i<=3; i++){
    digitalWrite(_pins[i], LOW);
  }

}

// -----------------------------------------------------------------
// run()
//
// This is the heart of the library. Call every loop() iteration.
//
// What this function must do:
//   1. If _stepFlag is false, return immediately (no step is due).
//   2. Clear _stepFlag (set it back to false).
//   3. If _running is false, return (nothing to do).
//   4. If _speedMode is true (constant-speed / jog mode):
//        - Advance _seqIndex by _moveDir, wrapping correctly within
//          [0, _seqLen).
//        - Advance _currentPos by _moveDir.
//        - Set _targetPos = _currentPos (keeps distanceToGo() == 0
//          for the whole duration of the jog, not just at entry).
//        - Call applyCoils().
//      Otherwise (position mode):
//        - Compute togo = _targetPos - _currentPos.
//        - If togo == 0, call stop() and return — the move is done.
//        - Otherwise, set _moveDir based on the sign of togo, advance
//          _seqIndex and _currentPos the same way as above, and call
//          applyCoils().
//
// Be careful with the wraparound arithmetic on _seqIndex — it must
// stay a valid, non-negative index into _seqTable for both directions
// of _moveDir.
// -----------------------------------------------------------------
void StepperISR::run() {
  // TODO: implement per the steps above
  // 
  if (_stepFlag == false){
      return;
  }

  _stepFlag = false;

  if (isRunning()== false){
    return;
  }

  if (_speedMode == true){
    _seqIndex = abs((_seqIndex + _moveDir)) % _seqLen;
    _currentPos = _currentPos + _moveDir;
    applyCoils();
  }
  else{
    if(distanceToGo() == 0){
      stop();
      return;
    }
    else{
      (distanceToGo() > 0) ? _moveDir = 1 : _moveDir = -1;
      _seqIndex = abs((_seqIndex + _moveDir)) % _seqLen;
      _currentPos = _currentPos + _moveDir;
      applyCoils();
    }
  }
}
