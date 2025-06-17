# Modified Surveillance Bot Hardware Guide

This guide outlines the essential hardware components and their connections for the Modified Surveillance Bot project, reflecting the changes made during the optimization process.

## 1. NodeMCU ESP8266 (Master Controller)

-   **Purpose:** Controls overall bot logic, Wi-Fi communication (Blynk), and communicates with the Arduino Uno Slave.
-   **Key Connections:**
    -   **D7 (GPIO13):** SoftwareSerial RX (connects to Arduino TX)
    -   **D8 (GPIO15):** SoftwareSerial TX (connects to Arduino RX)
    -   **SDA (GPIO4):** I2C Data (for OLED Display)
    -   **SCL (GPIO5):** I2C Clock (for OLED Display)

## 2. Arduino Uno (Slave Controller)

-   **Purpose:** Manages sensors (PIR, IR, Ultrasonic), motor drivers, servo, and speaker.
-   **Key Connections:**
    -   **Digital Pin 0 (RX):** Hardware Serial RX (connects to NodeMCU D8/TX via voltage divider if needed)
    -   **Digital Pin 1 (TX):** Hardware Serial TX (connects to NodeMCU D7/RX via voltage divider if needed)
    -   **Digital Pin 2 (PIR Sensor):** Input for PIR motion sensor.
    -   **Digital Pin 3 (IR Sensor):** Input for IR obstacle sensor.

    -   **Digital Pin 4 (SPEAKER_PIN):** Output for the first speaker (PWM capable).
-   **Digital Pin 8 (SPEAKER_PIN_2):** Output for the second speaker (PWM capable).
    -   **Digital Pin 5 (TRIG_PIN):** Output for Ultrasonic sensor trigger.
    -   **Digital Pin 6 (ECHO_PIN):** Input for Ultrasonic sensor echo.
    -   **Digital Pin 9 (SERVO_PIN):** PWM output for the servo motor.
    -   **Digital Pins 10, 11, 12, 13 (Motor Driver IN1, IN2, IN3, IN4):** Outputs for controlling motor direction and speed via an L298N or similar motor driver.
    -   **Analog Pin A0 (BUZZER):** Output for the buzzer.
    -   **Analog Pin A1 (LED_RED):** Output for Red LED.
    -   **Analog Pin A2 (LED_GREEN):** Output for Green LED.
    -   **Analog Pin A3 (LED_BLUE):** Output for Blue LED.

## 3. OLED Display (SSD1306)

-   **Purpose:** Provides visual feedback on bot status, sensor readings, and alerts.
-   **Connection:** I2C (SDA, SCL) to NodeMCU.

## 4. L298N Motor Driver Module

-   **Purpose:** Drives two DC motors for bot movement.
-   **Connections:**
    -   **IN1, IN2, IN3, IN4:** Connect to Arduino Digital Pins 10, 11, 12, 13 respectively.
    -   **OUT1, OUT2:** Connect to Motor 1.
    -   **OUT3, OUT4:** Connect to Motor 2.
    -   **12V/5V Power Input:** Connect to external power supply for motors.
    -   **GND:** Connect to common ground with Arduino.

## 5. SG90 Servo Motor

-   **Purpose:** Controls the direction of the ultrasonic sensor for scanning.
-   **Connections:**
    -   **Signal Pin:** Connect to Arduino Digital Pin 9.
    -   **VCC:** Connect to Arduino 5V.
    -   **GND:** Connect to Arduino GND.

## 6. HC-SR04 Ultrasonic Sensor

-   **Purpose:** Measures distance to obstacles.
-   **Connections:**
    -   **VCC:** Connect to Arduino 5V.
    -   **GND:** Connect to Arduino GND.
    -   **Trig Pin:** Connect to Arduino Digital Pin 5.
    -   **Echo Pin:** Connect to Arduino Digital Pin 6.

## 7. PIR Motion Sensor

-   **Purpose:** Detects motion.
-   **Connections:**
    -   **VCC:** Connect to Arduino 5V.
    -   **GND:** Connect to Arduino GND.
    -   **OUT:** Connect to Arduino Digital Pin 2.

## 8. IR Obstacle Sensor

-   **Purpose:** Detects nearby obstacles.
-   **Connections:**
    -   **VCC:** Connect to Arduino 5V.
    -   **GND:** Connect to Arduino GND.
    -   **OUT:** Connect to Arduino Digital Pin 3.

## 9. Speakers

-   **Purpose:** Provide audio feedback and alerts, with one speaker for left audio and one for right audio.
-   **Connections:**
    -   **Speaker 1 (Left):** Connect one lead to Arduino Digital Pin 4. Connect the other lead to Arduino GND (possibly through a current-limiting resistor).
    -   **Speaker 2 (Right):** Connect one lead to Arduino Digital Pin 8. Connect the other lead to Arduino GND (possibly through a current-limiting resistor).

## 10. LEDs (Red, Green, Blue)

-   **Purpose:** Visual indicators for bot status and alerts.
-   **Connections:**
    -   Connect anode (long leg) to Arduino Analog Pins A1, A2, A3 (via a current-limiting resistor).
    -   Connect cathode (short leg) to Arduino GND.

## 11. Buzzer

-   **Purpose:** Provides audible alerts.
-   **Connections:**
    -   Connect one lead to Arduino Analog Pin A0.
    -   Connect the other lead to Arduino GND.

## Wiring Considerations:

-   **Voltage Dividers:** When connecting NodeMCU (3.3V logic) to Arduino Uno (5V logic) for serial communication, use a voltage divider on the Arduino TX to NodeMCU RX line to protect the NodeMCU.
-   **Common Ground:** Ensure all components share a common ground connection.
-   **External Power:** A single, adequate external power supply (e.g., a 9V or 12V DC power adapter with sufficient current rating) should be used to power the entire system. This supply will typically connect to the L298N Motor Driver Module's 12V/5V input terminals. The motor driver often has a built-in 5V regulator that can then supply power to the Arduino Uno's 5V pin or VIN pin (if the input voltage is within its acceptable range). The Arduino Uno, in turn, can supply 5V to the NodeMCU's VIN pin or directly to its 5V pin if available, or the NodeMCU can be powered independently via its micro-USB port. Ensure all components share a common ground connection. It is crucial that the power supply can provide enough current for all components, especially the motors when they are actively running, to prevent voltage drops and erratic behavior.

## 12. Blynk Setup

Blynk is used for remote monitoring, control, and notifications. Here's how to set it up:

### In the Blynk App:

1.  **Create a New Project:** Open the Blynk app, create a new project, and select your device (e.g., ESP8266).
2.  **Get Auth Token:** An authentication token will be sent to your email. This token is crucial for connecting your NodeMCU to the Blynk server.
3.  **Add Widgets:** Add widgets to your project dashboard as needed. For this bot, you might consider:
    *   **Buttons:** For manual control (e.g., start/stop patrol, trigger alert).
    *   **Displays (e.g., Labeled Value, Gauge):** To show sensor readings (distance, motion status, temperature).
    *   **Notifications:** To receive alerts when an anomaly is detected.
    *   **Terminal:** For debugging messages from the bot.

### In the Code (`nodemcu_master.ino`):

1.  **Include Libraries:** Ensure you have the necessary Blynk libraries included:
    ```cpp
    #include <BlynkSimpleEsp8266.h>
    #include <ESP8266WiFi.h>
    ```
2.  **Blynk Credentials:** Update the `auth`, `ssid`, and `pass` variables with your actual Blynk authentication token, Wi-Fi network name (SSID), and Wi-Fi password. These are typically found near the beginning of the `nodemcu_master.ino` file:
    ```cpp
    char auth[] = "YOUR_BLYNK_AUTH_TOKEN"; // Your Blynk Auth Token
    char ssid[] = "YOUR_WIFI_SSID";       // Your WiFi network name
    char pass[] = "YOUR_WIFI_PASSWORD";     // Your WiFi password
    ```
3.  **Blynk.begin():** In the `setup()` function, initialize Blynk with your credentials:
    ```cpp
    Blynk.begin(auth, ssid, pass);
    ```
4.  **Blynk.run():** In the `loop()` function, ensure `Blynk.run()` is called frequently to maintain the connection and process incoming commands/data:
    ```cpp
    Blynk.run();
    ```
5.  **Virtual Pins:** Use `Blynk.virtualWrite(Vn, value);` to send data from your bot to Blynk widgets (where `Vn` is the virtual pin number configured in the app) and `BLYNK_WRITE(Vn)` functions to receive commands from Blynk widgets.

This guide provides a basic overview. Always refer to the datasheets of individual components for detailed specifications and connection diagrams.