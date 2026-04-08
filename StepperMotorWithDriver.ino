// ── StylBot Arduino – DRV8825 Stepper Controller ──────────────
// Receives commands from Raspberry Pi over USB Serial (9600 baud)
//
// Wiring (match the diagram):
//   Arduino D2  → DRV8825 STEP
//   Arduino D3  → DRV8825 DIR
//   Arduino D4  → DRV8825 SLP (HIGH = awake, LOW = sleep)
//   Arduino D5  → DRV8825 RST (keep HIGH)
//   Arduino GND → DRV8825 GND (logic side)
//   Arduino 5V  → DRV8825 VDD (logic power)
//   External PSU (8–36V) → DRV8825 VMOT + decoupling cap 100µF
//   External PSU GND     → DRV8825 GND (motor side)
//
// Motor coils:
//   DRV8825 2B → B+
//   DRV8825 2A → B−  (swap if motor spins wrong way)
//   DRV8825 1A → A+
//   DRV8825 1B → A−
//
// MS1, MS2, MS3 all LOW = full step (200 steps/rev)
// ──────────────────────────────────────────────────────────────
#include <Stepper.h>

#define STEP_PIN  2
#define DIR_PIN   3
#define SLP_PIN   4
#define RST_PIN   5

#define STEPS 100

const int   STEPS_PER_REV = 200;   // full step; multiply by microstep factor if MS pins set
const int   BPM_MIN       = 10;
const int   BPM_MAX       = 200;

Stepper stepper(STEPS, 2, 3, 4, 5);

int  bpm          = 60;
bool running      = false;

unsigned long stepInterval = 0;  // µs between steps
unsigned long lastStepTime = 0;
bool          stepState    = false;

// For NEXT / PREV fixed moves
long moveStepsLeft = 0;
int  moveDir       = 1;

String inputBuffer = "";

// ── Helpers ───────────────────────────────────────────────────
void recalcInterval() {
  stepInterval = 60000000UL / ((unsigned long)bpm * STEPS_PER_REV);
}

void wake()  { digitalWrite(SLP_PIN, HIGH); delayMicroseconds(2); }
//void wake() { stepper.setspeed(0); delayMicroseconds(2); }
void sleep() { digitalWrite(SLP_PIN, LOW);  }

void handleCommand(const String& cmd) {

  if (cmd == "STOP") {
    running       = false;
    moveStepsLeft = 0;
    sleep();
    Serial.println("[INFO] STOP");

  } else if (cmd == "RESUME") {
    running = true;
    wake();
    Serial.println("[INFO] RESUME");

  } else if (cmd == "NEXT") {
    wake();
    moveStepsLeft = STEPS_PER_REV;
    moveDir       = 1;
    digitalWrite(DIR_PIN, HIGH);
    Serial.println("[INFO] NEXT – 1 rev CW");

  } else if (cmd == "PREV") {
    wake();
    moveStepsLeft = STEPS_PER_REV;
    moveDir       = -1;
    digitalWrite(DIR_PIN, LOW);
    Serial.println("[INFO] PREV – 1 rev CCW");

  } else if (cmd.startsWith("BPM:+")) {
    bpm = constrain(bpm + cmd.substring(5).toInt(), BPM_MIN, BPM_MAX);
    recalcInterval();
    Serial.print("[INFO] BPM="); Serial.println(bpm);

  } else if (cmd.startsWith("BPM:-")) {
    bpm = constrain(bpm - cmd.substring(5).toInt(), BPM_MIN, BPM_MAX);
    recalcInterval();
    Serial.print("[INFO] BPM="); Serial.println(bpm);

  } else {
    Serial.print("[WARN] Unknown: "); Serial.println(cmd);
  }
}

// ── Setup ─────────────────────────────────────────────────────
void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);
  pinMode(SLP_PIN,  OUTPUT);
  pinMode(RST_PIN,  OUTPUT);

  digitalWrite(RST_PIN, HIGH);  // keep out of reset
  digitalWrite(DIR_PIN, HIGH);  // default: CW
  sleep();                       // start in sleep (motor off)
  recalcInterval();

  Serial.begin(9600);
  Serial.println("[INFO] StylBot DRV8825 ready");
}

// ── Loop ──────────────────────────────────────────────────────
void loop() {
  // Read serial commands
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      inputBuffer.trim();
      if (inputBuffer.length() > 0) {
        handleCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }

  // Non-blocking step timing
  unsigned long now = micros();
  if (now - lastStepTime >= stepInterval / 2) {
    lastStepTime = now;

    if (moveStepsLeft > 0) {
      // Fixed move (NEXT / PREV) takes priority
      stepState = !stepState;
      digitalWrite(STEP_PIN, stepState);
      if (!stepState) {          // count on falling edge = 1 full step
        moveStepsLeft--;
        if (moveStepsLeft == 0) {
          sleep();               // done — sleep to save power
          // restore CW direction for next continuous run
          digitalWrite(DIR_PIN, HIGH);
        }
      }
    } else if (running) {
      // Continuous rotation
      stepState = !stepState;
      digitalWrite(STEP_PIN, stepState);
    }
  }
}