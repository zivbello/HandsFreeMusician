#include <AccelStepper.h>
#include <Servo.h>

// ─────────────────────────────
// Stepper Motor Class Wrapper
// ─────────────────────────────
class StepperMotor {
  public:
    AccelStepper stepper;
    long stepsPerRev;

    StepperMotor(int stepPin, int dirPin, long stepsPerRev)
      : stepper(AccelStepper::DRIVER, stepPin, dirPin),
        stepsPerRev(stepsPerRev) {}

    void begin() {
      stepper.setMaxSpeed(800);
      stepper.setAcceleration(300);
    }

    long degreesToSteps(float deg) {
      return (deg / 360.0) * stepsPerRev;
    }

    void moveDegrees(float deg) {
      stepper.move(degreesToSteps(deg));
    }

    void moveDegreesRelative(float deg) {
      stepper.move(degreesToSteps(deg));
    }

    void stop() {
      stepper.stop(); // smooth deceleration
    }

    void emergencyStop() {
      stepper.setCurrentPosition(stepper.currentPosition());
    }

    void update() {
      stepper.run();
    }

    bool isMoving() {
      return stepper.distanceToGo() != 0;
    }
};

// ─────────────────────────────
// Pins
// ─────────────────────────────
#define STEP_PIN 2
#define DIR_PIN  3
#define SLP_PIN  4
#define RST_PIN  5

// ─────────────────────────────
// Objects
// ─────────────────────────────
StepperMotor motor(STEP_PIN, DIR_PIN, 200);
Servo myservo;

// ─────────────────────────────
// Serial
// ─────────────────────────────
String inputBuffer = "";

// ─────────────────────────────
// HIGH-LEVEL COMMAND HANDLER
// ─────────────────────────────
void handleCommand(String cmd) {
  cmd.trim();

  // ── STOP ──
  if (cmd == "STOP") {
    motor.stop();

    myservo.write(45);
    digitalWrite(SLP_PIN, LOW);

    Serial.println("[INFO] STOP");
  }

  // ── RESUME ──
  else if (cmd == "RESUME") {
    digitalWrite(SLP_PIN, HIGH);
    myservo.write(90);

    Serial.println("[INFO] RESUME");
  }

  // ── RIGHT ──
  else if (cmd.startsWith("RIGHT")) {
    float deg = cmd.substring(6).toFloat();
    motor.moveDegrees(deg);

    Serial.print("[INFO] RIGHT ");
    Serial.println(deg);
  }

  // ── LEFT ──
  else if (cmd.startsWith("LEFT")) {
    float deg = cmd.substring(5).toFloat();
    motor.moveDegrees(-deg);

    Serial.print("[INFO] LEFT ");
    Serial.println(deg);
  }

  else {
    Serial.print("[WARN] Unknown: ");
    Serial.println(cmd);
  }
}

// ─────────────────────────────
// SETUP
// ─────────────────────────────
void setup() {
  pinMode(SLP_PIN, OUTPUT);
  pinMode(RST_PIN, OUTPUT);

  digitalWrite(SLP_PIN, HIGH);
  digitalWrite(RST_PIN, HIGH);

  myservo.attach(9);

  Serial.begin(9600);
  Serial.println("[INFO] StylBot ready");

  motor.begin();
}

// ─────────────────────────────
// LOOP
// ─────────────────────────────
void loop() {

  // ── Serial Input ──
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

  // ── Motor update (non-blocking) ──
  motor.update();

  // ── Optional: auto-log completion ──
  if (!motor.isMoving()) {
    // you could use this for state logic if needed
  }
}
