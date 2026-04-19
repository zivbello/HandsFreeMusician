# Week 4
Group 8 Members: Ziven Bello, Kimberly Talisse, Leon Chen
___

## Mon, Apr 13 (12:35 - 3:35)
- Tested the chain connectors
- Found a new solution for connected the gantry carriage to the chain:
    - Using nuts and bolts to secure the plate to the chain 
- Tested the sprockets
- Figured out a solution for mounting the stepper motors and chain to the gantry:
    - Using plastic ball bearings and bolts to secure weights to the gantry arms
    - Using super glue to attach the motor casings to the weights
- Figured out a solution for attaching the electronics to the gantry
    - Using double-sided tape or similar adhesive to attach breadboard to back of gantry arm
- Tested the new servo motor

![Stepper Motor Casing](/images/Servo_Motor_Casing.jpg)

## Wed, Apr 15 (12:35 - 3:35)
- Marked out connection points between main gantry arm and support arms
- Discussed plans for project poster
- Attached motor casings to support arms
- Printed servo motor casing
- Remodeled servo motor casing: just scaled design up
- Modeled new stylus holder for servo to account for height
- Decided to change project direction
    - Now controlling the instrument directly rather than controlling preprogrammed songs
- Updated python and Arduino code to account for change in project idea

![Stylus Holder](/images/StylusHolder.jpg)

## Fri, Apr 17 (10:25 - 11:25)
- Began bolting together gantry arms
- Troubleshooted Arduino code for stepper motors
- Tested new pen holder and servo casing

## Fri, Apr 17 (12:40 - 3:40)
- Found issue with right motor casing connection
    - Super glue wore off, casing rotates resulting in chain being less taut
- Finished bolting together gantry arms
- Continued troubleshooting stepper motor code

## Sun Apr 19 (2:10 - 4:45)
- More troubleshooting stepper motors
- Got stepper motors working
- Secured gantry carriage to chain
- Secured servo to gantry carriage
- Secured loose stepper motor casing
- Updated Arduino code and python code to allow for note by note control

```C
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
```

```python
import argparse
import math
import time
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
from mediapipe.framework.formats import landmark_pb2
import serial

# ── Serial to Arduino ──────────────────────────────────────────
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)  # wait for Arduino to reset

# ── Gesture cooldown / hold state ─────────────────────────────
COOLDOWN     = 0.8
HOLD         = 0.4
SWIPE_THRESH = 0.25
BPM_MIN      = 60
BPM_MAX      = 180
bpm          = 120

last_trigger  = 0.0
last_static   = None
static_start  = 0.0
swipe_start_x = None
swipe_start_t = 0.0

def send(cmd):
    print(f"[CMD] {cmd}")
    ser.write((cmd + "\n").encode())

def get_command(gesture_name, wrist_x, now):
    global last_trigger, last_static, static_start
    global swipe_start_x, swipe_start_t, bpm

    cooldown_ok = (now - last_trigger) >= COOLDOWN

    # ── Static gestures (hold to trigger) ─────────────────
    static = None
    if gesture_name == "Closed_Fist":
        static = "fist"
    elif gesture_name == "Thumb_Up":
        static = "thumb_up"
    elif gesture_name == "Thumb_Down":
        static = "thumb_down"
    elif gesture_name == "Victory":
        static = "peace"

    if static:
        if static == last_static:
            if (now - static_start) >= HOLD and cooldown_ok:
                match static:
                    case "fist":
                        send("STOP")
                        last_trigger = now
                        return "Fist → STOP"
                    case "thumb_up":
                        send("LEFT 27")
                        last_trigger = now
                        return "Thumbs Up → LEFT"
                    case "thumb_down":
                        send("RIGHT 27")
                        last_trigger = now
                        return "Thumbs Down → RIGHT"
                    case "peace":
                        send("RESUME")
                        last_trigger = now
                        return "Peace → RESUME"
        else:
            last_static  = static
            static_start = now
    else:
        last_static = None

    return gesture_name if gesture_name != "None" else ""

# ── MediaPipe setup ────────────────────────────────────────────
mp_hands = mp.solutions.hands
mp_draw  = mp.solutions.drawing_utils

def run(model, num_hands, min_det, min_presence, min_track, camera_id, width, height):
    cap = cv2.VideoCapture(camera_id)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH,  width)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)

    base_options   = python.BaseOptions(model_asset_path=model)
    options        = vision.GestureRecognizerOptions(
        base_options=base_options,
        num_hands=num_hands,
        min_hand_detection_confidence=min_det,
        min_hand_presence_confidence=min_presence,
        min_tracking_confidence=min_track
    )
    recognizer = vision.GestureRecognizer.create_from_options(options)

    print("Running. Press Q to quit.")
    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame = cv2.flip(frame, 1)
        rgb   = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        now   = time.time()

        mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb)
        result = recognizer.recognize(mp_img)

        label = ""
        if result.gestures and result.hand_landmarks:
            gesture_name = result.gestures[0][0].category_name

            # Draw landmarks
            lm_list = result.hand_landmarks[0]
            proto   = landmark_pb2.NormalizedLandmarkList()
            proto.landmark.extend([
                landmark_pb2.NormalizedLandmark(x=l.x, y=l.y, z=l.z)
                for l in lm_list
            ])
            mp_draw.draw_landmarks(frame, proto, mp_hands.HAND_CONNECTIONS)

            # Wrist position + angle
            wrist_x = lm_list[0].x

            label = get_command(gesture_name, wrist_x, now)
        else:
            global prev_angle, rotation_acc, swipe_start_x
            swipe_start_x = None

        cv2.putText(frame, label, (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 1.0, (0, 255, 180), 2)
        cv2.imshow("StylBot Gesture", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model',                    default='gesture_recognizer.task')
    parser.add_argument('--numHands',       type=int, default=1)
    parser.add_argument('--minHandDetectionConfidence', type=float, default=0.5)
    parser.add_argument('--minHandPresenceConfidence',  type=float, default=0.5)
    parser.add_argument('--minTrackingConfidence',      type=float, default=0.5)
    parser.add_argument('--cameraId',       type=int, default=0)
    parser.add_argument('--frameWidth',     type=int, default=640)
    parser.add_argument('--frameHeight',    type=int, default=480)
    args = parser.parse_args()

    run(args.model, args.numHands,
        args.minHandDetectionConfidence,
        args.minHandPresenceConfidence,
        args.minTrackingConfidence,
        args.cameraId, args.frameWidth, args.frameHeight)

if __name__ == '__main__':
    main()
```
