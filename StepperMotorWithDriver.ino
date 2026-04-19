
// ── StylBot Arduino – StepStick (A4988/DRV8825) Controller ───
// Receives commands from Raspberry Pi over USB Serial (9600 baud)
//
// Wiring:
//   Arduino D2  → StepStick STEP
//   Arduino D3  → StepStick DIR
//   Arduino D4  → StepStick SLP  (HIGH = awake, LOW = sleep)
//   Arduino D5  → StepStick RST  (bridge to SLP or keep HIGH)
//   Arduino 5V  → StepStick VDD
//   Arduino GND → StepStick GND (logic side)
//   PSU +       → StepStick VMOT  (+ 100µF cap across VMOT/GND)
//   PSU −       → StepStick GND  (motor side, shared with Arduino GND)
//
// Motor coils (swap A or B pair if motor spins wrong direction):
//   StepStick 1A → A+
//   StepStick 1B → A−
//   StepStick 2A → B−
//   StepStick 2B → B+
//
// MS1, MS2, MS3 unconnected = full step (200 steps/rev)
// ──────────────────────────────────────────────────────────────
#include <Servo.h>

#define STEP_PIN  2
#define DIR_PIN   3
#define SLP_PIN   4
#define RST_PIN   5

Servo myservo;

const int   STEPS_PER_REV = 200;
const int   BPM_MIN       = 10;
const int   BPM_MAX       = 300;

int           bpm          = 30;
bool          running      = false;
unsigned long stepInterval = 0;
unsigned long lastStepTime = 0;
String        inputBuffer  = "";


void recalcInterval() {
  stepInterval = 60000000UL / ((unsigned long)bpm * STEPS_PER_REV);
}


void stepOnce(bool cw) {
  digitalWrite(DIR_PIN, cw ? HIGH : LOW);
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(STEP_PIN, LOW);
}

void wake()  { digitalWrite(SLP_PIN, HIGH); delayMicroseconds(2); }
void sleep() { digitalWrite(SLP_PIN, LOW); }

void handleCommand(const String& cmd) {
  if (cmd == "STOP") {
    running = false;
    myservo.write(45);
    sleep();
    Serial.println("[INFO] STOP");

  } else if (cmd == "RESUME") {
    running = true;
    wake();
    myservo.write(90);
    Serial.println("[INFO] RESUME");

  } else if (cmd.startsWith("LEFT")) {
    // make half step ccw

      //for (int i = 0; i < STEPS_PER_REV; i++) {
        stepOnce(false);
        delayMicroseconds(stepInterval - 5);
      //}
      //digitalWrite(DIR_PIN, HIGH);
      Serial.print("[INFO] LEFT");

  } else if (cmd.startsWith("RIGHT")) {
    //make half step cw
      //for (int i = 0; i < STEPS_PER_REV; i++) {
        stepOnce(true);
        delayMicroseconds(stepInterval - 5);
      //}
      Serial.print("[INFO] RIGHT");

  } else {
    Serial.print("[WARN] Unknown: "); Serial.println(cmd);
  }
}

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);
  pinMode(SLP_PIN,  OUTPUT);
  pinMode(RST_PIN,  OUTPUT);

  // added servo now
  myservo.attach(9);

  digitalWrite(RST_PIN, HIGH);
  digitalWrite(DIR_PIN, HIGH);
  sleep();
  recalcInterval();

  Serial.begin(9600);
  Serial.println("[INFO] StylBot ready");
}

void loop() {
  // Read serial commands from Pi
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

  // Non-blocking step pulse
  
  if (running) {
    unsigned long now = micros();
    if (now - lastStepTime >= stepInterval) {
      lastStepTime = now;
      stepOnce(true);
      
      //digitalWrite(STEP_PIN, HIGH);
      //delayMicroseconds(5);
      //digitalWrite(STEP_PIN, LOW);
    }
  }
}