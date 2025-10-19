# Introduction to Robotics (2025-2026)

## Laboratory Homeworks

Faculty of Mathematics and Computer Science, University of Bucharest
3rd Year – Robotics Specialization

This repository contains the laboratory homeworks completed for the Introduction to Robotics course.
Each homework includes the official requirements, source files, implementation details, images of the physical setup, and a demonstration video.



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

