#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ESP32Servo.h>

// DHT11 setup
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LED pins
const int ledGreenPin = 23;
const int ledYellowPin = 22;
const int ledRedPin = 21;

// Button pin
const int buttonPin = 5;

// Servo setup
const int servoPin = 18;
Servo myServo;

// State variables
unsigned long buttonPressTime = 0;
bool buttonWasPressed = false;
bool autoMode = false;
float lastTemperature = 20;

// Numerical method variables (Euler Method)
float previousTemperature = 20;
float estimatedTemperature = 20;
const float deltaT = 2.0; // seconds

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(ledGreenPin, OUTPUT);
  pinMode(ledYellowPin, OUTPUT);
  pinMode(ledRedPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  // Setup servo
  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);
  myServo.write(90); // Initial neutral position
}

void loop() {
  handleButton();

  if (autoMode) {
    static unsigned long lastReadTime = 0;
    if (millis() - lastReadTime > 2000) {
      lastReadTime = millis();
      readTemperatureAndControlLED();
    }
  }

  controlServo();
}

void handleButton() {
  if (digitalRead(buttonPin) == LOW && !buttonWasPressed) {
    buttonPressTime = millis();
    buttonWasPressed = true;
  }

  if (digitalRead(buttonPin) == HIGH && buttonWasPressed) {
    unsigned long pressDuration = millis() - buttonPressTime;
    buttonWasPressed = false;

    if (pressDuration >= 2000) {
      autoMode = !autoMode;
      Serial.print("Auto Mode: ");
      Serial.println(autoMode ? "ON" : "OFF");
    } else {
      if (!autoMode) {
        readTemperatureAndControlLED();
      }
    }
  }
}

void readTemperatureAndControlLED() {
  float temperature = dht.readTemperature();

  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Euler's Method to estimate temperature trend
  float dT = (temperature - previousTemperature) / deltaT;
  estimatedTemperature = temperature + dT * deltaT;
  previousTemperature = temperature;
  lastTemperature = temperature;

  // Print actual and estimated temperatures
  Serial.print("Current Temp: ");
  Serial.print(temperature);
  Serial.print(" °C | Estimated Temp (Euler): ");
  Serial.print(estimatedTemperature);
  Serial.println(" °C");

  // LED control based on current temperature
  if (temperature < 22) {
    digitalWrite(ledGreenPin, HIGH);
    digitalWrite(ledYellowPin, LOW);
    digitalWrite(ledRedPin, LOW);
  } else if (temperature >= 20 && temperature <= 25) {
    digitalWrite(ledGreenPin, LOW);
    digitalWrite(ledYellowPin, HIGH);
    digitalWrite(ledRedPin, LOW);
  } else {
    digitalWrite(ledGreenPin, LOW);
    digitalWrite(ledYellowPin, LOW);
    digitalWrite(ledRedPin, HIGH);
  }
}

// Servo control logic
void controlServo() {
  if (lastTemperature > 25) {
    myServo.write(0);   // Full clockwise
  } else if (lastTemperature >= 20 && lastTemperature <= 25) {
    myServo.write(120); // Slow counter-clockwise
  } else {
    myServo.write(90);  // Stop
  }
}
