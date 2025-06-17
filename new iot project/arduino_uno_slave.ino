// Arduino Uno Slave Sketch for SURVEILLANCE BOT
// This sketch receives commands from a NodeMCU (running the main logic)
// and controls hardware accordingly (motors, sensors, servo, speaker, LEDs).

#include <Servo.h>

// Pin definitions (ensure these match your actual Uno wiring)
#define RED_LED 7
#define GREEN_LED 8
#define BLUE_LED 2 // Example pin, adjust as needed
#define BUZZER 9
#define SERVO_PIN 10
#define PIR_SENSOR 6
#define TRIG_PIN 11
#define ECHO_PIN 12
#define IR_SENSOR 5
#define SPEAKER_PIN 4
#define SPEAKER_PIN_2 8 // Define a new pin for the second speaker

// Motor driver pins
#define ENA 3
#define ENB 4
#define IN1 A0
#define IN2 A1
#define IN3 A2
#define IN4 A3

// Objects
Servo scanner;

#define INPUT_BUFFER_SIZE 200
char inputBuffer[INPUT_BUFFER_SIZE];
uint8_t inputPos = 0;
bool stringComplete = false;     // Whether the string is complete

void setup() {
  Serial.begin(115200); // Match baud rate with NodeMCU
  Serial.println("Arduino Uno Slave Ready.");

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(PIR_SENSOR, INPUT);
  pinMode(IR_SENSOR, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(SPEAKER_PIN_2, OUTPUT); // Initialize the second speaker pin

  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  digitalWrite(GREEN_LED, HIGH); // Default to green LED on
  digitalWrite(RED_LED, LOW);
  stopMotors();

  scanner.attach(SERVO_PIN);
  scanner.write(90); // Center servo
}

void loop() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    serialCharTimestamp = millis(); // Update timestamp with each char received
    if (inputPos < INPUT_BUFFER_SIZE - 1) {
      inputBuffer[inputPos++] = inChar;
      inputBuffer[inputPos] = '\0';
    } else {
      Serial.println("ERR: Serial buffer overflow, command too long.");
      inputBuffer[0] = '\0';
      inputPos = 0;
      stringComplete = false;
      break;
    }
    if (inChar == '\n') {
      stringComplete = true;
      break;
    }
  }
  // Timeout check: if no char received for a while, and string is not empty but not complete
  if (!stringComplete && inputPos > 0 && (millis() - serialCharTimestamp > serialTimeout)) {
    Serial.print("ERR: Serial command timeout. Incomplete: ");
    Serial.println(inputBuffer);
    inputBuffer[0] = '\0';
    inputPos = 0;
  }

  if (stringComplete) {
    Serial.print("Uno Received: ");
    Serial.println(inputBuffer);
    processCommand(inputBuffer);
    inputBuffer[0] = '\0';
    inputPos = 0;
    stringComplete = false;
  }
}

void processCommand(const char* command) {
  // Defensive: check for null or empty
  if (!command || command[0] == '\0') return;

  // Command validation and parsing
  if (strncmp(command, "CMD:MOTOR:FORWARD", 18) == 0) {
    moveForward();
  } else if (strncmp(command, "CMD:MOTOR:BACKWARD", 19) == 0) {
    moveBackward();
  } else if (strncmp(command, "CMD:MOTOR:LEFT", 15) == 0) {
    turnLeft();
  } else if (strncmp(command, "CMD:MOTOR:RIGHT", 16) == 0) {
    turnRight();
  } else if (strncmp(command, "CMD:MOTOR:STOP", 15) == 0) {
    stopMotors();
  } else if (strncmp(command, "CMD:SERVO:SET_ANGLE:", 20) == 0) {
    int angle = atoi(command + 20);
    if (angle >= 0 && angle <= 180) {
      scanner.write(angle);
    } else {
      Serial.println("ERR: Invalid servo angle");
    }
  } else if (strncmp(command, "CMD:SERVO:SWEEP_START", 21) == 0) {
    scanner.write(0);
  } else if (strncmp(command, "CMD:SERVO:SWEEP_STOP", 20) == 0) {
    scanner.write(90);
  } else if (strncmp(command, "CMD:ALERT:START", 15) == 0) {
    triggerAlertSound();
  } else if (strncmp(command, "CMD:ALERT:STOP", 14) == 0) {
    digitalWrite(BUZZER, LOW);
    noTone(SPEAKER_PIN);
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
  } else if (strncmp(command, "CMD:SPEAKER:PLAY:", 17) == 0) {
    playNamedSound(command + 17);
  } else if (strncmp(command, "CMD:SPEAKER:TONE:", 17) == 0) {
    // CMD:SPEAKER:TONE:frequency:duration
    const char* freqPtr = command + 17;
    const char* colonPtr = strchr(freqPtr, ':');
    if (colonPtr) {
      int frequency = atoi(freqPtr);
      int duration = atoi(colonPtr + 1);
      if (frequency > 0 && duration > 0) {
        tone(SPEAKER_PIN, frequency, duration);
      } else {
        Serial.println("ERR: Invalid speaker tone params");
      }
    }
  } else if (strncmp(command, "CMD:SPEAKER:TONE_2:", 19) == 0) {
    // CMD:SPEAKER:TONE_2:frequency:duration
    const char* freqPtr = command + 19;
    const char* colonPtr = strchr(freqPtr, ':');
    if (colonPtr) {
      int frequency = atoi(freqPtr);
      int duration = atoi(colonPtr + 1);
      if (frequency > 0 && duration > 0) {
        tone(SPEAKER_PIN_2, frequency, duration);
      } else {
        Serial.println("ERR: Invalid speaker 2 tone params");
      }
    }
  } else if (strncmp(command, "CMD:SPEAKER:STOP", 16) == 0) {
    noTone(SPEAKER_PIN);
  } else if (strncmp(command, "CMD:SPEAKER:STOP_2", 18) == 0) {
    noTone(SPEAKER_PIN_2);
  } else if (strncmp(command, "CMD:LED:RED:ON", 14) == 0) {
    digitalWrite(RED_LED, HIGH);
  } else if (strncmp(command, "CMD:LED:RED:OFF", 15) == 0) {
    digitalWrite(RED_LED, LOW);
  } else if (strncmp(command, "CMD:LED:GREEN:ON", 16) == 0) {
    digitalWrite(GREEN_LED, HIGH);
  } else if (strncmp(command, "CMD:LED:GREEN:OFF", 17) == 0) {
    digitalWrite(GREEN_LED, LOW);
  } else if (strncmp(command, "CMD:LED:BLUE:ON", 15) == 0) {
    digitalWrite(BLUE_LED, HIGH);
  } else if (strncmp(command, "CMD:LED:BLUE:OFF", 16) == 0) {
    digitalWrite(BLUE_LED, LOW);
  } else if (strncmp(command, "REQ:ALL_SENSORS", 15) == 0) {
    sendAllSensorData();
  } else if (strncmp(command, "REQ:DISTANCE", 12) == 0) {
    Serial.print("DATA:DIST:");
    Serial.println(getDistance());
  } else if (strncmp(command, "REQ:PIR", 7) == 0) {
    Serial.print("DATA:PIR:");
    Serial.println(digitalRead(PIR_SENSOR));
  } else if (strncmp(command, "REQ:IR", 6) == 0) {
    Serial.print("DATA:IR:");
    Serial.println(digitalRead(IR_SENSOR));
  } else if (strncmp(command, "CMD:PING", 8) == 0) {
    Serial.println("ACK:PONG");
  } else {
    Serial.print("Unknown command: ");
    Serial.println(command);
  }
}

void sendAllSensorData() {
  // Send data in a format NodeMCU can parse easily
  // Example: "DATA:PIR:1;IR:0;DIST:150;SERVO:90"
  String dataPacket = "DATA:";
  dataPacket += "PIR:";
  dataPacket += String(digitalRead(PIR_SENSOR));
  dataPacket += ";IR:";
  dataPacket += String(digitalRead(IR_SENSOR));
  dataPacket += ";DIST:";
  dataPacket += String(getDistance());
  dataPacket += ";SERVO:";
  dataPacket += String(scanner.read());
  Serial.println(dataPacket);
}

int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 25000); // 25ms timeout
  if (duration == 0) return 0; // Return 0 for timeout/no echo
  return duration * 0.0343 / 2;
}

void triggerAlertSound() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, 1500, 150);
    tone(SPEAKER_PIN, 2000, 150);
    tone(SPEAKER_PIN_2, 2500, 150); // Use the second speaker
    unsigned long startDelay = millis();
  while (millis() - startDelay < 200) {}
    noTone(BUZZER);
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2); // Stop the second speaker
    unsigned long startDelay = millis();
  while (millis() - startDelay < 100) {}
  }
}

void playNamedSound(const char* soundName) {
  // This is where you'd map soundName to specific tone sequences
  if (strcmp(soundName, "PATROL") == 0) {
    tone(SPEAKER_PIN, 880, 150); unsigned long startDelay = millis(); while (millis() - startDelay < 180) {} // Left speaker
    tone(SPEAKER_PIN_2, 880, 150); startDelay = millis(); while (millis() - startDelay < 180) {} // Right speaker
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  } else if (strcmp(soundName, "INTRUDER") == 0) {
    for (int i = 0; i < 5; i++) {
      tone(SPEAKER_PIN, 1000, 100); // Left speaker
      tone(SPEAKER_PIN_2, 1000, 100); // Right speaker
      unsigned long startDelay = millis();
  while (millis() - startDelay < 120) {}
      tone(SPEAKER_PIN, 1500, 100); // Left speaker
      tone(SPEAKER_PIN_2, 1500, 100); // Right speaker
      unsigned long startDelay = millis();
       while (millis() - startDelay < 120) {}
    }
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  } else if (strcmp(soundName, "SCANNING") == 0) {
    tone(SPEAKER_PIN, 400, 100); unsigned long startDelay = millis(); while (millis() - startDelay < 120) {} // Left speaker
    tone(SPEAKER_PIN_2, 400, 100); startDelay = millis(); while (millis() - startDelay < 120) {} // Right speaker
    tone(SPEAKER_PIN, 600, 100); startDelay = millis(); while (millis() - startDelay < 120) {} // Left speaker
    tone(SPEAKER_PIN_2, 600, 100); startDelay = millis(); while (millis() - startDelay < 120) {} // Right speaker
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  } else if (strcmp(soundName, "STARTUP") == 0) {
    tone(SPEAKER_PIN, 523, 200); unsigned long startDelay = millis(); while (millis() - startDelay < 220) {} // C - Left speaker
    tone(SPEAKER_PIN_2, 523, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // C - Right speaker
    tone(SPEAKER_PIN, 659, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // E - Left speaker
    tone(SPEAKER_PIN_2, 659, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // E - Right speaker
    tone(SPEAKER_PIN, 784, 300); startDelay = millis(); while (millis() - startDelay < 320) {} // G - Left speaker
    tone(SPEAKER_PIN_2, 784, 300); startDelay = millis(); while (millis() - startDelay < 320) {} // G - Right speaker
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  } else if (strcmp(soundName, "SHUTDOWN") == 0) {
    tone(SPEAKER_PIN, 784, 200); unsigned long startDelay = millis(); while (millis() - startDelay < 220) {} // G - Left speaker
    tone(SPEAKER_PIN_2, 784, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // G - Right speaker
    tone(SPEAKER_PIN, 659, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // E - Left speaker
    tone(SPEAKER_PIN_2, 659, 200); startDelay = millis(); while (millis() - startDelay < 220) {} // E - Right speaker
    tone(SPEAKER_PIN, 523, 300); startDelay = millis(); while (millis() - startDelay < 320) {} // C - Left speaker
    tone(SPEAKER_PIN_2, 523, 300); startDelay = millis(); while (millis() - startDelay < 320) {} // C - Right speaker
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  } else {
    // Default beep for unknown sounds
    tone(SPEAKER_PIN, 800, 200); // Left speaker
    tone(SPEAKER_PIN_2, 800, 200); // Right speaker
    unsigned long startDelay = millis();
  while (millis() - startDelay < 220) {}
    noTone(SPEAKER_PIN);
    noTone(SPEAKER_PIN_2);
  }
}

// Motor control functions
void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 255); // Full speed
  analogWrite(ENB, 255); // Full speed
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 255); // Full speed
  analogWrite(ENB, 255); // Full speed
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 200); // Medium speed
  analogWrite(ENB, 200); // Medium speed
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, 200); // Medium speed
  analogWrite(ENB, 200); // Medium speed
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}