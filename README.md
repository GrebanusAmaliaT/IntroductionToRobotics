# Introduction to Robotics (2025-2026)

## Laboratory Homeworks

Faculty of Mathematics and Computer Science, University of Bucharest
3rd Year – Robotics Specialization

This repository contains the laboratory homeworks completed for the Introduction to Robotics course.
Each homework includes the official requirements, source files, implementation details, images of the physical setup, and a demonstration video.

<br>

## Homework 3 - Home Alarm System

### Task Requirements
Design and implement an Arduino-based home alarm system that uses an ultrasonic sensor (HC-SR04) and a photoresistor (LDR) to detect intrusions or changes in the environment.
The system can be armed, disarmed, and configured through a Serial Monitor menu.
When armed, it continuously monitors the environment and triggers an alarm if a movement or a sudden distance variation is detected.

### Implementation Details
The circuit consists of:
1 × Ultrasonic sensor (HC-SR04) connected to pins Trig = 6, Echo = 7
1 × Photoresistor (LDR) connected to A0
1 × Buzzer connected to pin 5
1 × Red LED (alarm indicator) on pin 10
1 × Green LED (disarmed indicator) on pin 11
Resistors and jumper wires as needed

### Functional description:

#### When powered on, the system:
- Initializes the Serial Monitor and prints a welcome message with the system name.
- Automatically calibrates the ultrasonic sensor by taking several distance readings to establish a baseline (“normal” distance).
- Enters the Disarmed state by default, indicated by the green LED.
- Displays the Main Menu for user interaction through the Serial Monitor.
- System Menu (via Serial Monitor)

#### If Disarmed:
Arm System – manually arms the alarm.
The system enters a 3-second arming phase before becoming active.
Settings / Configuration – opens a submenu that allows parameter adjustment.
Test Alarm – manually activates the alarm for testing purposes.

#### Settings Submenu:
1. Set Ultrasonic Sensitivity (distantaMax)
Defines the distance variation threshold (in cm) that triggers the alarm when exceeded.
2. Set LDR Light Threshold (pragLDR)
Sets the light level under which the system arms automatically (“night mode”).
3. Set Buzzer Frequency (buzzerFrecventa)
Changes the pitch of the alarm sound (in Hz).
4. Change Password
Requires entering the current password before setting a new one.
5. Change System Name
Updates the displayed name at startup.
6. Return to Main Menu

#### If Armed:
The system continuously monitors both sensors.
If a significant distance change is detected by the ultrasonic sensor, or the light level is below threshold, the system:
Warns of a possible intrusion.
Prompts the user to enter the password within 3 seconds.
If the password is correct → Disarm system.
If incorrect or not entered → Trigger alarm (buzzer + red LED blinking).

### System Behavior Summary
| State                            | Description                                                                              | Indicators                  |
| -------------------------------- | ---------------------------------------------------------------------------------------- | --------------------------- |
| **Dezarmat (Disarmed)**          | System inactive. User can arm manually or wait for dark conditions to arm automatically. | Green LED ON                |
| **SeArmeaza (Arming)**           | 3-second countdown before activation.                                                    | Green LED blinking          |
| **Armat (Armed)**                | System actively monitors distance and light sensors.                                     | LEDs off, silent            |
| **PregatitDeActivare (Warning)** | Intrusion detected. Waits for password for 3 seconds.                                    | Serial Monitor prompt       |
| **AlarmaActiva (Alarm)**         | Alarm triggered (sound + red LED blinking).                                              | Red LED flashing, buzzer-ON |


### Code Structure and Style
The program is written in modular form, using clear functions for each subsystem:
- masurareDistanta() – reads the ultrasonic sensor and returns distance in centimeters.
- setareUltrasonic() – performs automatic calibration at startup.
- heckSensors() – reads both sensors and updates system state.
- handleMenu() – manages user interaction in the main menu.
- handleSettingsMenu() – manages the configuration submenu.
- handlePassword() – verifies and validates entered passwords.
- updateAlarm() – controls buzzer and red LED blinking when alarm is active.
- afisareMeniu() and afisareSubmeniuSetari() – display menu text and structure.

All time control is handled using millis() for non-blocking delays, ensuring continuous sensor monitoring and responsive menu navigation.

### System Logic Flow
- Startup & Calibration
The system averages multiple ultrasonic readings to set an initial reference distance.
- Disarmed Mode
The green LED stays ON; if light drops below the pragLDR value, the system arms automatically.
- Arming Delay (3 seconds)
Gives the user time to leave before activation.
- Monitoring Phase
If distance variation > distantaMax, the system waits for a password.
- Password Window (3 seconds)
If password not entered or incorrect → alarm activates.
- Alarm State
Buzzer emits sound at buzzerFrecventa; red LED blinks until the correct password is entered.
- Disarm
Stops buzzer, turns off red LED, turns on green LED, and returns to idle state.

## Sistem Feedback
Serial Monitor provides all interactions and confirmations:
- “Sistemul a fost armat.”
- “SPY CAT a detectat un posibil intrus! Introduceti parola.”
- “Parola corecta. Sistem dezarmat.”
- “Parola gresita – alarma activata!”
Invalid commands in the menu trigger clear messages and re-display of the available options.

### Pictures of the Setup
....
### Video
....

<br>

## Homework 2 – Traffic Lights System

### Task Requirements
Design and implement an Arduino-based traffic light control system for a pedestrian crosswalk. The system coordinates the behavior of two traffic lights: one for cars and one for pedestrians. It uses a push button to request pedestrian crossing, a buzzer for auditory feedback, and a 7-segment display to show countdown timers. The transition between traffic light states must follow the specified sequence and timing, using non-blocking logic with millis() instead of delay().

### Implementation Details

#### The circuit consists of:

- Three LEDs for vehicles: red, yellow, and green, connected to digital pins 10, 9, and 8 respectively.
- Two LEDs for pedestrians: red and green, connected to pins 3 and 5.
- One push button (pin 2) for pedestrians to request a crossing. The button is configured with an internal pull-up resistor and handled using an interrupt that sets a flag when pressed.
- One buzzer (pin 6) that provides auditory cues during the pedestrian crossing phases.
- A 7-segment display, connected through pins 7, 4, 11, 12, 13, A5, A4, and A3, which shows countdown values during the timed states.

#### The program is structured as a finite state machine with four main states:

1. Idle State (Default)
Cars: green, Pedestrians: red
Buzzer: off
Duration: indefinite — waits for a valid button press.
The display is cleared or can remain off.

2. Transition State (Yellow Light)
Begins 8 seconds after a valid button press.
Cars: yellow, Pedestrians: red
Buzzer: off
Duration: 3 seconds
Countdown shown on the 7-segment display.

3. Pedestrian Green State (Crossing Allowed)
Cars: red, Pedestrians: green
Duration: 8 seconds
Countdown shown on the display.
The buzzer emits slow beeps (1 Hz) to signal the crossing time.

4. Warning State (Blinking Green)
Cars: red, Pedestrians: blinking green
Duration: 4 seconds
Countdown shown on the display.
Buzzer emits rapid beeps (~3 Hz) to warn that the crossing time is ending.
After this state, the system automatically returns to Idle.

All timing is managed using the millis() function to ensure non-blocking behavior. 
This allows accurate countdown display and real-time responsiveness to transitions. 
The interrupt service routine (ISR) attached to the button pin detects falling edges, 
setting a flag to initiate the state sequence only if the system is in the Idle state.

### Code Structure and Style

The program defines clear constants for all pin assignments and timing values. Descriptive variable names such as se_face_galben, se_face_rosu, and warning indicate each active phase. Modular functions handle specific subtasks:

- afisareNumaratoare() – displays digits on the 7-segment module.
- stergereNumaratoare() – clears the display.
- idle() – resets the system to its default state.
- se_apasa_butonul() – interrupt function for button press detection.

The logic in loop() continuously checks time differences using millis() to transition smoothly between states. No delay() is used, ensuring responsiveness and correct timing.

### System Behavior Summary
- When the system starts, vehicles have a green light and pedestrians a red light. Pressing the pedestrian button triggers a timed sequence:
- After 8 seconds, the vehicle light turns yellow for 3 seconds.
- Vehicles then stop, and pedestrians get a green light with a slow buzzer and countdown.
- The pedestrian green light begins blinking and the buzzer speeds up for 4 seconds.
- Finally, the system resets to its initial idle state.

### Pictures of the Setup
![WhatsApp Image 2025-10-27 at 22 16 43_e516a140](https://github.com/user-attachments/assets/b1b62aec-ab23-4ad0-b8f9-008b2f3c405f)
![WhatsApp Image 2025-10-27 at 22 16 43_3e49c2fd](https://github.com/user-attachments/assets/c0cd73f4-df40-4694-bdad-0ccf35edf8bf)
![WhatsApp Image 2025-10-27 at 22 16 43_5752864a](https://github.com/user-attachments/assets/2b56ecdd-5386-494b-a992-71f703f0be1a)

### Video
https://youtube.com/shorts/KFcf_d8wKIo?feature=share

## Homework 1 - RGB LED Control with 3 Potentiometers

### Task Requirements

Create an Arduino circuit and program that controls an RGB LED using three potentiometers — one for each color component (red, green, and blue). Each potentiometer should adjust the brightness of its respective color, allowing the LED to display any color combination.

The potentiometer values must be read through analog inputs and mapped to the 0–255 range required by the PWM output. The implementation should use clear variable names and constants instead of “magic numbers,” and the code must follow proper formatting and consistent style.

### Implementation details 
The circuit uses three potentiometers connected to the analog pins A0, A1 and A2 of the Arduino UNO. Each potentiometer controls one color channel of a common-cathode RGB LED, whose red, green, and blue pins are connected through appropriate resistors to PWM-enabled digital pins 9, 10 and 11.

The Arduino program continuously reads the analog voltage from each potentiometer, then maps those readings from the input range 0–1023 to the PWM output range 0–255 using the map() function. The resulting values are written to the LED pins with analogWrite(), which adjusts the intensity of each color channel. By varying the three potentiometers, the user can mix colors in real time to produce the full RGB spectrum.

All numerical constants, such as the maximum analog or PWM values, are defined at the beginning of the code for readability and precision. The sketch maintains consistent indentation, descriptive variable names, and comments explaining each step, following the provided style guide.

### Pictures of the setup
![poza1](https://github.com/user-attachments/assets/0f9ed50d-454f-4ad5-b0aa-eea64735bf96)
![poza2](https://github.com/user-attachments/assets/fe00b1e4-353c-4bbe-9034-7f97eff80574)
![poza3](https://github.com/user-attachments/assets/8cd44f28-21f4-46c5-8f19-329ba0a78f0a)

### Video
https://youtube.com/shorts/D9-72uJa4h8?feature=share

<br>
