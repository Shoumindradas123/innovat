# Surveillance Bot: Behavior and Environmental Interaction

This document outlines the intended behavior and interaction mechanisms of the Surveillance Bot within its environment.

## Core Functionality

The bot is designed to autonomously patrol a designated area, detect anomalies, and alert users to potential threats. Its primary functions include:

1.  **Patrolling:** The bot will navigate a predefined path or explore its environment using its motor system, avoiding obstacles.
2.  **Sensor Monitoring:** Continuously collect data from various sensors (ultrasonic, PIR, IR) to detect movement, proximity, and heat signatures.
3.  **Anomaly Detection:** Analyze sensor data to identify unusual activity or intrusions.
4.  **Alerting:** Upon detecting an anomaly, the bot will trigger an alert mechanism, which may include visual (LEDs), auditory (speaker, buzzer), and remote notifications (via Blynk).
5.  **Response Actions:** Depending on the detected anomaly, the bot may initiate specific actions, such as tracking movement with its servo-mounted sensor or moving to a specific location.

## Environmental Interaction

The bot interacts with its environment through its sensors and actuators:

### Input (Sensors)

*   **Ultrasonic Sensor:** Used for obstacle avoidance and distance measurement, allowing the bot to navigate without collisions.
*   **PIR Sensor:** Detects motion, triggering the bot to investigate potential intruders.
*   **IR Sensor:** Can be used for line following, edge detection, or short-range obstacle detection.
*   **Thermal Sensor (if integrated):** Detects heat signatures, useful for identifying living beings in low-light conditions or through certain materials.

### Output (Actuators)

*   **Motors (via Motor Driver):** Enable the bot's movement for patrolling, pursuit, or repositioning.
*   **Servo Motor:** Allows for directional scanning with sensors (e.g., ultrasonic or thermal) to cover a wider area or track moving objects.
*   **OLED Display:** Provides local visual feedback on the bot's status, detected events, or sensor readings.
*   **Speaker/Buzzer:** Generates audible alerts or sound sequences to deter intruders or signal specific events.
*   **LEDs:** Provide visual indicators for different states (e.g., patrolling, alert, charging).

### Communication

*   **Serial Communication (NodeMCU & Arduino Uno):** The NodeMCU (master) communicates with the Arduino Uno (slave) to offload sensor data processing and control of certain peripherals (e.g., motors, speaker).
*   **Blynk (NodeMCU):** The NodeMCU connects to the internet via Wi-Fi and uses the Blynk platform for remote monitoring, control, and notifications. This allows users to receive alerts, view sensor data, and potentially control the bot from a smartphone or web interface.

## Operational Flow (High-Level)

1.  **Initialization:** Upon power-up, the bot initializes its sensors, actuators, and establishes communication with the Arduino Uno and Blynk.
2.  **Patrol Mode:** The bot enters a patrolling state, moving along its designated path or exploring. During this phase, it continuously monitors sensor data.
3.  **Detection:** If a sensor (e.g., PIR, ultrasonic) detects an anomaly (motion, close proximity), the bot transitions to an investigation or alert state.
4.  **Investigation/Response:** The bot may stop, orient its sensors towards the detected anomaly, and gather more data. If the anomaly is confirmed as a threat, it triggers an alarm (sound, lights) and sends a notification via Blynk.
5.  **Return to Patrol:** After the threat is no longer detected or a predefined response action is completed, the bot returns to its patrolling routine.

This structured approach ensures the bot can effectively monitor its environment, react to events, and communicate with the user.