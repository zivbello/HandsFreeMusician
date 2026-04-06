# Week 2
Group 8 Members: Ziven Bello, Kimberly Talisse, Leon Chen
___

## Mon, Mar 30 (12:30 - 3:30)
- Redesigned pen holder to better fit the stylus and have a longer arm
- Tried to install the computer vision algorithms we found into a virtual environment but they both were too large
- The second algorithm was built for ASL interpreting rather than general hand gestures, so we will not use that one
- Researching more algorithms to work with
- found this hand gesture database: https://www.kaggle.com/datasets/gti-upm/leapgestrecog/data
- Found deep-learning gesture recognition model: https://github.com/guillaumephd/deep_learning_hand_gesture_recognition?tab=readme-ov-file
- Considering running models on our own laptops to avoid issues with installing mediapipe on R-Pi
- Also found this model: https://github.com/fabiopk/RT_GestureRecognition
- Modeled several potential gears for the chain
- Got mediapipe working on a laptop, will need to train for more gestures or find another model that uses mediapipe
- Will work on refining gesture recognition and implement communication with Arduino
- Will figure out (if possible) how to get code running on R-Pi
- Got a thumb drive to help with prototyping 3d printed parts

![3D Model of penholder prototype and gears](/images/prototypeholder3andgears.png "3D Model of penholder prototype and gears")

## Mon, Apr 1 (12:30 - 3:30)
- Changed the hand gesture to: Thumbs Up → BPM +10, Thumbs Down → BPM -10, Fist → Stop, Peace → Resume
- Used Mediapipe on a personal computer and got the gesture and camera working
- Copied gesture code and model file to Raspberry Pi, gesture recognition running live on Raspberry Pi camera now
- Printed out prototype parts in preparation for arrival of ordered parts
- Programmed the Arduino for testing gesture control
- Got the Pi to communicate with the Arduino
- Attached servo to Arduino to test stylus holder
- Need new servo as the lab ones are far too weak and overheat easily
- Ordered new servo

```C 

#define LED_GREEN   8   // RUNNING / RESUME
#define LED_RED     9   // STOP
#define LED_YELLOW  10  // NEXT / PREV / BPM change


int  bpm     = 60;
bool running = false;

String inputBuffer = "";

void flashYellow(int times = 1) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_YELLOW, HIGH);
    delay(100);
    digitalWrite(LED_YELLOW, LOW);
    delay(100);
  }
}

void handleCommand(const String& cmd) {
  if (cmd == "STOP") {
    running = false;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED,   HIGH);
    Serial.println("[INFO] STOP");

  } else if (cmd == "RESUME") {
    running = true;
    digitalWrite(LED_RED,   LOW);
    digitalWrite(LED_GREEN, HIGH);
    Serial.println("[INFO] RESUME");

  } else if (cmd == "NEXT") {
    flashYellow(2);
    Serial.println("[INFO] NEXT");

  } else if (cmd == "PREV") {
    flashYellow(3);
    Serial.println("[INFO] PREV");

  } else if (cmd.startsWith("BPM:+")) {
    bpm = min(bpm + cmd.substring(5).toInt(), 300);
    flashYellow(1);
    Serial.print("[INFO] BPM="); Serial.println(bpm);

  } else if (cmd.startsWith("BPM:-")) {
    bpm = max(bpm - cmd.substring(5).toInt(), 10);
    flashYellow(1);
    Serial.print("[INFO] BPM="); Serial.println(bpm);

  } else {
    Serial.print("[WARN] Unknown: "); Serial.println(cmd);
  }
}

void setup() {
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);

  // Startup blink
  digitalWrite(LED_GREEN,  HIGH); delay(200);
  digitalWrite(LED_RED,    HIGH); delay(200);
  digitalWrite(LED_YELLOW, HIGH); delay(200);
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_RED,    LOW);
  digitalWrite(LED_YELLOW, LOW);

  Serial.begin(9600);
  Serial.println("[INFO] StylBot LED prototype ready");
}

void loop() {
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

    # ── Swipe (track wrist x, only when no static gesture) ─
    global swipe_start_x, swipe_start_t
    STATIC_GESTURES = ("Closed_Fist", "Thumb_Up", "Thumb_Down", "Pointing_Up")
    is_static = gesture_name in STATIC_GESTURES
    if not is_static:
        if swipe_start_x is None:
            swipe_start_x = wrist_x
            swipe_start_t = now
        else:
            delta   = wrist_x - swipe_start_x
            elapsed = now - swipe_start_t
            if elapsed < 0.8 and cooldown_ok:
                if delta > SWIPE_THRESH:
                    send("NEXT")
                    last_trigger  = now
                    swipe_start_x = None
                    return "Swipe Right → NEXT"
                elif delta < -SWIPE_THRESH:
                    send("PREV")
                    last_trigger  = now
                    swipe_start_x = None
                    return "Swipe Left → PREV"
            elif elapsed >= 0.8:
                swipe_start_x = wrist_x
                swipe_start_t = now
    else:
        swipe_start_x = None

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
                        if bpm + 10 <= BPM_MAX:
                            bpm += 10
                            send(f"BPM:+10")
                            last_trigger = now
                            return f"Thumbs Up → BPM UP ({bpm})"
                        else:
                            return f"BPM MAX ({BPM_MAX})"
                    case "thumb_down":
                        if bpm - 10 >= BPM_MIN:
                            bpm -= 10
                            send(f"BPM:-10")
                            last_trigger = now
                            return f"Thumbs Down → BPM DOWN ({bpm})"
                        else:
                            return f"BPM MIN ({BPM_MIN})"
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

## Fri, Apr 3 (10:25 - 11:25)
- Reviewed the overall project plan and inspect the parts that just arrived
- Researched gear designing
- Found website for generating sprocket gears https://meta-matic.com/en/api/generate-sprocket/
- Found website for converting .step/.stp into .stl files

## Fri, Apr 3 (12:25 - 3:35)
- Printed out sprockets
- Modeled prototype casing for stepper motors
- Printed out prototype casing
- Researched gantry designs

![3D Model of Sprocket](/images/sprocketroundbore.jpg "3D Model of Sprocket with Round Bore")
![3D Model of Sprocket](/images/sprocketsquarebore.jpg "3D Model of Sprocket with Square Bore")
![3D Model of Casing](/images/steppermotorcasing.jpg "3D Model of Prototype Stepper Motor Casing")
