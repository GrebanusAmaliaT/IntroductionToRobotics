const int val_analog_min = 0;
const int val_analog_max = 1023;
const int val_pwm_min = 0;
const int val_pwm_max = 255;

const int pin_led_r = 9;
const int pin_led_v = 10;
const int pin_led_b = 11;

const int pin_in_r = A0;
const int pin_in_v = A1;
const int pin_in_b = A2;

const unsigned long intarziere_ms = 200;

void setup() {
  pinMode(pin_led_r, OUTPUT);
  pinMode(pin_led_v, OUTPUT);
  pinMode(pin_led_b, OUTPUT);

  Serial.begin(9600); 
  Serial.println();
  Serial.println("---------------------------------------Start---------------------------------------");
  Serial.println("Valorile sunt:\t");
}

void loop() {
  int val_in_r = analogRead(pin_in_r);
  int val_in_v = analogRead(pin_in_v);
  int val_in_b = analogRead(pin_in_b);

  int pwm_r = map(val_in_r, val_analog_min, val_analog_max, val_pwm_min, val_pwm_max);
  int pwm_v = map(val_in_v, val_analog_min, val_analog_max, val_pwm_min, val_pwm_max);
  int pwm_b = map(val_in_b, val_analog_min, val_analog_max, val_pwm_min, val_pwm_max);

  analogWrite(pin_led_r, pwm_r);
  analogWrite(pin_led_v, pwm_v);
  analogWrite(pin_led_b, pwm_b);

  Serial.print("Rosu: ");
  Serial.print(pwm_r);
  Serial.print("\tVerde: ");
  Serial.print(pwm_v);
  Serial.print("\tAlbastru: ");
  Serial.println(pwm_b);

  delay(intarziere_ms); 
}

