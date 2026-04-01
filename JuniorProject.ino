
/*
LED1 = 2;
LED2 = 3;

void setup() {
  // put your setup code here, to run once:
  pinMode(LED1, OUTPUT)
  pinMode(LED2, OUTPUT)
  Serial.begin(9600)
}

void loop() {
  // put your main code here, to run repeatedly:
  while(Serial.available){

  }
}
*/

// ── StylBot Arduino LED Prototype ─────────────────────────────
// Receives commands from Raspberry Pi over USB Serial (9600 baud)
//
// Wiring:
//   Green LED  → D8 (RUNNING indicator)
//   Red LED    → D9 (STOP indicator)
//   Yellow LED → D10 (NEXT/PREV flash)
//   All LED cathodes → GND via 220Ω resistor
// ──────────────────────────────────────────────────────────────

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