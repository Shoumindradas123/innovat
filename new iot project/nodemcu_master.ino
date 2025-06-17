#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCAN_RANGE 180

// Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SoftwareSerial arduinoSerial(D7, D8); // RX, TX to Arduino

// State machine
enum State { PATROL, SCAN, DETECTED, APPROACH, ALERT };
State currentState = PATROL;
State lastSpokenState = PATROL;

// Detection data structure
struct Detection {
  bool motion;
  int distance;
  int direction;
  bool confirmed;
};

// Structure for non-blocking sounds
struct SoundStep {
  int frequency;
  int duration;
  int postDelay;
};

// Global variables
Detection currentDetection = {false, 0, 90, false};
bool oled_initialized_successfully = false;
int servoPos = 90;
int scanDir = 1;
unsigned long lastScan = 0;
unsigned long detectionStart = 0;
unsigned long lastDisplay = 0;
int alertCount = 0;
unsigned long approachStart = 0;

// Blynk credentials (replace with your own)
// TODO: Replace with your actual Blynk Auth Token
char auth[] = "YOUR_BLYNK_AUTH_TOKEN";
// TODO: Replace with your actual WiFi SSID
char ssid[] = "YOUR_WIFI_SSID";
// TODO: Replace with your actual WiFi Password
char pass[] = "YOUR_WIFI_PASSWORD";

unsigned long lastBlynkConnectAttempt = 0;
const unsigned long blynkReconnectInterval = 5000;

bool blynkConnected = false; // Flag to track Blynk connection status

// Sound sequences for states
const SoundStep patrolSound[] = {{523, 100, 120}, {0,0,50}, {523, 100, 120}, {0,0,500}};
const int patrolSoundLength = sizeof(patrolSound) / sizeof(SoundStep);
const SoundStep scanSound[] = {{784, 80, 100}, {0,0,50}, {988, 80, 100}, {0,0,400}};
const int scanSoundLength = sizeof(scanSound) / sizeof(SoundStep);
const SoundStep detectedSound[] = {{1047, 150, 180}, {0,0,50}, {1047, 150, 180}, {0,0,300}};
const int detectedSoundLength = sizeof(detectedSound) / sizeof(SoundStep);
const SoundStep approachSound[] = {{659, 120, 150}, {0,0,50}, {523, 120, 150}, {0,0,50}, {659, 120, 150}, {0,0,400}};
const int approachSoundLength = sizeof(approachSound) / sizeof(SoundStep);
const SoundStep alertSound[] = {{1319, 200, 220}, {0,0,80}, {1319, 200, 220}, {0,0,80}, {1319, 200, 220}, {0,0,200}};
const int alertSoundLength = sizeof(alertSound) / sizeof(SoundStep);

// Initialization sound sequences
const SoundStep oledFailedSequence[] = {
  {300, 200, 220}, {0, 0, 100}, {300, 200, 220}, {0, 0, 100}, {100, 400, 420}, {0, 0, 100}
};
const int oledFailedSequenceLength = sizeof(oledFailedSequence) / sizeof(SoundStep);

const SoundStep botV1Sequence[] = {
  {784, 120, 140}, {988, 120, 140}, {1175, 180, 200}, {0, 0, 100}
};
const int botV1SequenceLength = sizeof(botV1Sequence) / sizeof(SoundStep);

const SoundStep initSequence[] = {
  {600, 80, 100}, {0, 0, 60}, {600, 80, 100}, {0, 0, 60}, {600, 80, 100}, {0, 0, 60}
};
const int initSequenceLength = sizeof(initSequence) / sizeof(SoundStep);

const SoundStep systemReadySequence[] = {
  {1046, 200, 220}, {1318, 200, 220}, {0, 0, 100}
};
const int systemReadySequenceLength = sizeof(systemReadySequence) / sizeof(SoundStep);

// Sound playback variables
unsigned long lastSoundTime = 0;
int currentSoundStep = 0;
const SoundStep* activeSoundSequence = nullptr;
int activeSoundSequenceLength = 0;

// Function prototypes
void sendCommandToSlave(String command);

void processSensorData(String data);
void checkSlaveConnection();
String readSlaveResponse(unsigned long timeout);
void handlePatrol();
void handleScan();
void handleDetected(unsigned long currentTime);
void handleApproach(unsigned long currentTime);
void handleAlert(unsigned long currentTime);
void updateDisplay();





void checkSlaveConnection() {
  Serial.println("Checking slave connection...");
  unsigned long startTime = millis();
  bool connected = false;
  const int maxRetries = 5;
  int retries = 0;

  while (retries < maxRetries) {
    Serial.print("Attempting to connect to slave (retry " + String(retries + 1) + ")...");
    sendCommandToSlave("CMD:PING");
    String response = readSlaveResponse(500); // Wait for 500ms for a response
    response.trim();

    if (response == "ACK:PONG") {
      Serial.println("Slave connected successfully!");
      connected = true;
      break;
    } else {
      Serial.println("No PONG response. Received: '" + response + "'");
      retries++;
      // Non-blocking delay for retry
    unsigned long startRetryDelay = millis();
    while (millis() - startRetryDelay < 200) {}
    }
  }

  if (!connected) {
    Serial.println("Failed to connect to slave after " + String(maxRetries) + " retries. Continuing without slave communication.");
    if(oled_initialized_successfully) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("SLAVE CONNECT FAILED");
      display.setCursor(0, 16);
      display.println("NO ARDUINO");
      display.display();
    }
    speakInit("OLED_FAILED");
  } else {
    if(oled_initialized_successfully) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("SLAVE CONNECTED");
      display.display();
      // Non-blocking delay for success message
  unsigned long startSuccessDisplay = millis();
  while (millis() - startSuccessDisplay < 1000) {}
    }
  }
}

void startSoundSequence(const SoundStep* sequence, int length) {
    activeSoundSequence = sequence;
    activeSoundSequenceLength = length;
    currentSoundStep = 0;
    lastSoundTime = 0;
}

void updateSpeaker() {
  if (activeSoundSequence == nullptr || currentSoundStep >= activeSoundSequenceLength) {
    if (activeSoundSequence != nullptr) {
        // Send command to slave to stop speaker
        sendCommandToSlave("CMD:SPEAKER:STOP");
        activeSoundSequence = nullptr;
        currentSoundStep = 0;
    }
    return;
  }

  unsigned long currentTime = millis();
  if (currentTime - lastSoundTime >= (unsigned long)(activeSoundSequence[currentSoundStep-1].duration + activeSoundSequence[currentSoundStep-1].postDelay) || currentSoundStep == 0) {
    // Send sound command to slave
    String soundCmd = "CMD:SPEAKER:TONE:" + String(activeSoundSequence[currentSoundStep].frequency) + ":" + String(activeSoundSequence[currentSoundStep].duration);
    sendCommandToSlave(soundCmd);
    lastSoundTime = currentTime;
    currentSoundStep++;
  }
}

void speakInit(const char* msg) {
  if(strcmp(msg, "OLED_FAILED") == 0) {
    startSoundSequence(oledFailedSequence, oledFailedSequenceLength);
  } else if(strcmp(msg, "BOT_V1") == 0) {
    startSoundSequence(botV1Sequence, botV1SequenceLength);
  } else if(strcmp(msg, "INIT") == 0) {
    startSoundSequence(initSequence, initSequenceLength);
  } else if(strcmp(msg, "READY") == 0) {
    startSoundSequence(systemReadySequence, systemReadySequenceLength);
  }
}

void speakPhrase(State stateToSpeak) {
  switch(stateToSpeak) {
    case PATROL:
      startSoundSequence(patrolSound, patrolSoundLength);
      break;
    case SCAN:
      startSoundSequence(scanSound, scanSoundLength);
      break;
    case DETECTED:
      startSoundSequence(detectedSound, detectedSoundLength);
      break;
    case APPROACH:
      startSoundSequence(approachSound, approachSoundLength);
      break;
    case ALERT:
      startSoundSequence(alertSound, alertSoundLength);
      break;
  }
}



void setup() {
  Serial.begin(115200); // Match baud rate with slave
  Serial.println("NodeMCU Master Starting...");

  arduinoSerial.begin(115200); // Initialize SoftwareSerial for Arduino communication

  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    speakInit("OLED_FAILED");
    oled_initialized_successfully = false;
  } else {
    oled_initialized_successfully = true;
    display.clearDisplay();
  }

  if(oled_initialized_successfully) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Servaillance Motion Bot v1.0");
    display.setCursor(0, 16);
    display.println("Initializing...");
    display.display();
  }

  speakInit("BOT_V1");
  // Non-blocking delay for initial sounds
  unsigned long startSoundTime = millis();
  while (millis() - startSoundTime < 1000) {
    updateSpeaker();
  }
  speakInit("INIT");
  startSoundTime = millis();
  while (millis() - startSoundTime < 2000) {
    updateSpeaker();
  }

  checkSlaveConnection();

  // Initialize slave

  sendCommandToSlave("CMD:LED:BLUE:ON");
   sendCommandToSlave("CMD:SERVO:SET_ANGLE:90");
  sendCommandToSlave("CMD:MOTOR:STOP");

  // Blynk setup
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiConnectStart < 30000) { // Try for 30 seconds
    // Non-blocking delay for WiFi connection
    unsigned long currentMillis = millis();
    static unsigned long lastDotTime = 0;
    if (currentMillis - lastDotTime >= 500) {
      Serial.print(".");
      lastDotTime = currentMillis;
    }
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected, IP: " + WiFi.localIP().toString());
    Blynk.begin(auth, ssid, pass);
    blynkConnected = true;
  } else {
    Serial.println("WiFi connection failed. Continuing without Blynk.");
    if(oled_initialized_successfully) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("WIFI FAILED");
      display.setCursor(0, 16);
      display.println("NO BLYNK");
      display.display();
      // Non-blocking delay for WiFi failed message
      unsigned long startDisplayTime = millis();
      while (millis() - startDisplayTime < 2000) {}
    }
  }

  Serial.println("System Ready!");
  speakInit("READY");
  currentState = PATROL;
}

void loop() {
  // Automatic control logic will go here
  // For now, just keep Blynk running if connected and update speaker
  if (blynkConnected) {
    Blynk.run();
  }
  updateSpeaker();

  // Process slave responses
  if (arduinoSerial.available()) {
    String slaveResponse = arduinoSerial.readStringUntil('\n');
    slaveResponse.trim();
    Serial.print("Master received: ");
    Serial.println(slaveResponse);
    
    if (slaveResponse.startsWith("DATA:")) {
      processSensorData(slaveResponse);
    }
  }

  // Handle state changes
  if(currentState != lastSpokenState) {
    speakPhrase(currentState);
    lastSpokenState = currentState;
  }

  // State machine
  unsigned long currentTime = millis();
  switch(currentState) {
    case PATROL:
      handlePatrol();
      break;
    case SCAN:
      handleScan();
      break;
    case DETECTED:
      handleDetected(currentTime);
      break;
    case APPROACH:
      handleApproach(currentTime);
      break;
    case ALERT:
      handleAlert(currentTime);
      break;
  }

  // Update display
  if(oled_initialized_successfully && (currentTime - lastDisplay > 1000)) { // Update display every 1 second
    updateDisplay();
    lastDisplay = currentTime;
  }

  // Request sensor data periodically
  // The requestSensorData function is now called within the state handlers to avoid blocking delays.
  // requestSensorData();

  // Non-blocking delay to prevent busy-waiting
  unsigned long startBusyWaitDelay = millis();
  while (millis() - startBusyWaitDelay < 10) {}
}

// Request all sensor data from slave
void requestSensorData() {
  sendCommandToSlave("REQ:ALL_SENSORS");
}

void sendCommandToSlave(String command) {
  Serial.println("Sending to Uno: " + command);
  arduinoSerial.println(command);
}

String readSlaveResponse(unsigned long timeout) {
  unsigned long startTime = millis();
  String response = "";
  while (millis() - startTime < timeout) {
    if (arduinoSerial.available()) {
      char inChar = arduinoSerial.read();
      response += inChar;
      if (inChar == '\n') {
        return response;
      }
    }
  }
  return response; // Return whatever was read, even if incomplete
}




void processSensorData(String data) {
  // Parse: "DATA:PIR:1;IR:0;DIST:150;SERVO:90"
  if (data.indexOf("PIR:") != -1) {
    int pirStart = data.indexOf("PIR:") + 4;
    int pirEnd = data.indexOf(";", pirStart);
    if (pirEnd == -1) pirEnd = data.length();
    bool pir = data.substring(pirStart, pirEnd).toInt() == 1;
    
    int irStart = data.indexOf("IR:") + 3;
    int irEnd = data.indexOf(";", irStart);
    if (irEnd == -1) irEnd = data.length();
    bool ir = data.substring(irStart, irEnd).toInt() == 1;
    
    int distStart = data.indexOf("DIST:") + 5;
    int distEnd = data.indexOf(";", distStart);
    if (distEnd == -1) distEnd = data.length();
    int dist = data.substring(distStart, distEnd).toInt();
    
    // Update current detection
    currentDetection.motion = pir || ir;
    currentDetection.distance = dist;
  }
}

void handlePatrol() {
  // Request sensor data from slave
  requestSensorData();
  static unsigned long lastMovementTime = 0;
  static unsigned long movementDuration = 0;
  static bool moving = false;

  if (!moving) {
    requestSensorData();
    if((currentDetection.distance > 0 && currentDetection.distance < 25) || currentDetection.motion) {
      sendCommandToSlave("CMD:MOTOR:STOP");
      lastMovementTime = millis();
      movementDuration = 200;
      moving = true;
    } else {
      sendCommandToSlave("CMD:MOTOR:FORWARD");
      lastMovementTime = millis();
      movementDuration = 400;
      moving = true;
    }
  } else {
    if (millis() - lastMovementTime >= movementDuration) {
      if (movementDuration == 200) { // After stopping for 200ms
        sendCommandToSlave("CMD:MOTOR:RIGHT");
        lastMovementTime = millis();
        movementDuration = 600;
      } else if (movementDuration == 600) { // After turning right for 600ms
        sendCommandToSlave("CMD:MOTOR:STOP");
        lastMovementTime = millis();
        movementDuration = 200;
      } else if (movementDuration == 400) { // After moving forward for 400ms
        sendCommandToSlave("CMD:MOTOR:STOP");
        lastMovementTime = millis();
        movementDuration = 100;
      } else if (movementDuration == 100) { // After stopping for 100ms
        moving = false;
      }
    }
  }

  // Periodic scanning
  if(millis() - lastScan > 4000) {
    startScan();
  }
}

void handleScan() {
  requestSensorData();
  static unsigned long lastScanMoveTime = 0;
  const unsigned long scanMoveInterval = 300;

  requestSensorData();

  // Check for detection
  if(currentDetection.motion) {
    currentDetection.direction = servoPos;
    currentDetection.confirmed = false;
    Serial.println("Motion detected during scan!");
    currentState = DETECTED;
    detectionStart = millis();
    return;
  }

  // Continue scanning
  if (millis() - lastScanMoveTime >= scanMoveInterval) {
    servoPos += scanDir * 10;
    if(servoPos >= 180) {
      servoPos = 180;
      scanDir = -1;
    } else if(servoPos <= 0) {
      servoPos = 0;
      scanDir = 1;
      finishScan();
      return;
    }
    sendCommandToSlave("CMD:SERVO:SET_ANGLE:" + String(servoPos));
    lastScanMoveTime = millis();
  }
}

void handleDetected(unsigned long currentTime) {
  static unsigned long lastSensorRequestTime = 0;
  const unsigned long sensorRequestInterval = 100;

  if(currentTime - detectionStart < 2000) {
    if (millis() - lastSensorRequestTime >= sensorRequestInterval) {
      requestSensorData();
      lastSensorRequestTime = millis();
    }
    
    if(currentDetection.motion) {
      Serial.println("Confirming detection...");
    } else {
      Serial.println("Detection lost");
      currentState = PATROL;
      sendCommandToSlave("CMD:SERVO:SET_ANGLE:90");
      servoPos = 90;
    }
  } else {
    currentDetection.confirmed = true;
    currentState = APPROACH;
    approachStart = millis();
    // Reset static variables for handleApproach()
    static int approachDir = 1; // Reset approachDir
    approachDir = 1;
    static unsigned long backwardStartTime = 0; // Reset backwardStartTime
    backwardStartTime = 0;
    static unsigned long movementStartTime = 0; // Reset movementStartTime
    movementStartTime = 0;
    static unsigned long movementDuration = 0; // Reset movementDuration
    movementDuration = 0;
    static bool movementActive = false; // Reset movementActive
    movementActive = false;
    Serial.println("Target confirmed, approaching");
  }
}

void handleApproach(unsigned long currentTime) {
  static unsigned long lastSensorRequestTime = 0;
  const unsigned long sensorRequestInterval = 100;

  if (millis() - lastSensorRequestTime >= sensorRequestInterval) {
    requestSensorData();
    lastSensorRequestTime = millis();
  }
  
  bool obstacle = (currentDetection.distance > 0 && currentDetection.distance < 30);
  static int approachDir = 1;
  static unsigned long lastServoMove = 0;

  // Timeout check
  if(currentTime - approachStart > 15000) {
    Serial.println("Approach timeout");
    currentState = PATROL;
    sendCommandToSlave("CMD:SERVO:SET_ANGLE:90");
    servoPos = 90;
    return;
  }

  // Distance-based behavior
  if(currentDetection.distance < 40 && currentDetection.distance > 0) {
    sendCommandToSlave("CMD:MOTOR:STOP");
    // Non-blocking backward movement
    static unsigned long backwardStartTime = 0;
    if (backwardStartTime == 0) {
      sendCommandToSlave("CMD:MOTOR:BACKWARD");
      backwardStartTime = millis();
    }
    if (millis() - backwardStartTime >= 600) {
      sendCommandToSlave("CMD:MOTOR:STOP");
      backwardStartTime = 0;
    }
    Serial.println("Too close, retreating");
    currentState = PATROL;
    return;
  } else if(currentDetection.distance >= 50 && currentDetection.distance <= 80) {
    sendCommandToSlave("CMD:MOTOR:STOP");
    triggerAlert();
    currentState = ALERT;
    Serial.println("Alert range reached");
    return;
  } else if(currentDetection.distance > 80 || currentDetection.distance == 0) {
    // Movement logic
    // Non-blocking movement logic
    static unsigned long movementStartTime = 0;
    static unsigned long movementDuration = 0;
    static bool movementActive = false;

    if (!movementActive) {
      if(obstacle) {
        sendCommandToSlave("CMD:MOTOR:STOP");
        movementStartTime = millis();
        movementDuration = 200; // Stop for 200ms
        movementActive = true;
      } else {
        sendCommandToSlave("CMD:MOTOR:FORWARD");
        movementStartTime = millis();
        movementDuration = 400; // Move forward for 400ms
        movementActive = true;
      }
    } else {
      if (millis() - movementStartTime >= movementDuration) {
        if (movementDuration == 200) { // After stopping for 200ms, turn right
          sendCommandToSlave("CMD:MOTOR:RIGHT");
          movementStartTime = millis();
          movementDuration = 600; // Turn right for 600ms
        } else if (movementDuration == 600) { // After turning right for 600ms, stop
          sendCommandToSlave("CMD:MOTOR:STOP");
          movementActive = false;
        } else if (movementDuration == 400) { // After moving forward for 400ms, stop
          sendCommandToSlave("CMD:MOTOR:STOP");
          movementActive = false;
        }
      }
    }
    
    // Servo tracking
    if(currentTime - lastServoMove > 400) {
      servoPos = (approachDir == 1) ? 120 : 60;
      sendCommandToSlave("CMD:SERVO:SET_ANGLE:" + String(servoPos));
      approachDir = 1 - approachDir;
      lastServoMove = currentTime;
    }
  }
}

void handleAlert(unsigned long currentTime) {
  static unsigned long alertStart = 0;
  
  if(alertStart == 0) {
    alertStart = currentTime;
  }

  if(currentTime - alertStart > 8000) {
    sendCommandToSlave("CMD:LED:BLUE:OFF");
    sendCommandToSlave("CMD:LED:GREEN:ON");
    sendCommandToSlave("CMD:ALERT:STOP");
    currentState = PATROL;
    sendCommandToSlave("CMD:SERVO:SET_ANGLE:90");
    servoPos = 90;
    alertStart = 0;
    Serial.println("Alert complete");
  } else {
    static unsigned long lastBeep = 0;
    if(currentTime - lastBeep > 800) {
      sendCommandToSlave("CMD:LED:BLUE:ON");
      sendCommandToSlave("CMD:ALERT:START");
      lastBeep = currentTime;
    }
  }
}

void startScan() {
  lastScan = millis();
  currentState = SCAN;
  servoPos = 90 - SCAN_RANGE/2;
  scanDir = 1;
  sendCommandToSlave("CMD:SERVO:SET_ANGLE:" + String(servoPos));
  sendCommandToSlave("CMD:MOTOR:STOP");
  Serial.println("Starting scan...");
}

void finishScan() {
  sendCommandToSlave("CMD:SERVO:SET_ANGLE:90");
  servoPos = 90;
  currentState = PATROL;
  Serial.println("Scan complete");
}

void triggerAlert() {
  alertCount++;
  sendCommandToSlave("CMD:LED:RED:ON");
  sendCommandToSlave("CMD:LED:GREEN:OFF");
  sendCommandToSlave("CMD:ALERT:START");

  Serial.println("*** MOTION ALERT ***");
  Serial.print("Distance: "); Serial.print(currentDetection.distance); Serial.println("cm");
  Serial.print("Direction: "); Serial.print(currentDetection.direction); Serial.println("deg");
  Serial.print("Alert #"); Serial.println(alertCount);
  Serial.println("********************");
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Status
  display.setCursor(0, 0);
  display.print("Status: ");
  switch(currentState) {
    case PATROL: display.println("PATROL"); break;
    case SCAN: display.println("SCAN"); break;
    case DETECTED: display.println("DETECT"); break;
    case APPROACH: display.println("APPROACH"); break;
    case ALERT: display.println("ALERT!"); break;
  }

  // Sensor readings
  display.setCursor(0, 12);
  display.print("Motion: ");
  display.print(currentDetection.motion ? "YES" : "NO");

  // Distance and servo position
  display.setCursor(0, 24);
  display.print("Dist: ");
  display.print(currentDetection.distance);
  display.println("cm");
  display.setCursor(70, 24);
  display.print("Servo: ");
  display.print(servoPos);

  // Alert count
  display.setCursor(0, 36);
  display.print("Alerts: ");
  display.print(alertCount);

  if(currentDetection.confirmed) {
    display.setCursor(0, 48);
    display.print("TARGET CONFIRMED!");
  }

  display.display();
}