/****************************************
 * Include Libraries
 ****************************************/
#include "UbidotsEsp32Mqtt.h"
#include <NewPing.h> // Ultrassonic Sensor

/****************************************
 * Define Constants
 ****************************************/
const char *UBIDOTS_TOKEN = "";  // Put here your Ubidots TOKEN
const char *WIFI_SSID = "";      // Put here your Wi-Fi SSID
const char *WIFI_PASS = "";      // Put here your Wi-Fi password
const char *DEVICE_LABEL = "esp32";
const char *HC_SR04_LABEL = "distance";
const char *THERMISTOR_NTC_LABEL = "temperature";
const char *PHOTORESISTOR_LDR_LABEL = "light";

const int PUBLISH_FREQUENCY = 10000; // Update rate in milliseconds

unsigned long timer;

Ubidots ubidots(UBIDOTS_TOKEN);

// Sensors
//// HC-SR04
#define TRIGGER_PIN 12
#define ECHO_PIN 13
#define MAX_DISTANCE 400
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
int distance;

//// Thermistor
const uint8_t THERMISTOR_PIN = 34;
float temperature;
const float RESISTANCE = 10000.0; 
const float VOLTAGE_UC = 3.3; 

//// Photoresistor 
const uint8_t PHOTORESISTOR_PIN = 35;
int light;

//// LEDs
const uint8_t GREEN_LED_PIN = 4;
const uint8_t YELLOW_LED_PIN = 16;
const uint8_t RED_LED_PIN = 17;

/****************************************
 * Auxiliar Functions
 ****************************************/

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

int get_distance() {
  const int DISTANCE_VALUE = sonar.ping_cm();

  return DISTANCE_VALUE;
}

float get_resistance() {
  const float ADC_RESOLUTION_UC = 4095.0;
  const float UNKNOWN_RESISTANCE = RESISTANCE * (VOLTAGE_UC / ((analogRead(THERMISTOR_PIN) * VOLTAGE_UC) / ADC_RESOLUTION_UC) - 1);

  return UNKNOWN_RESISTANCE;
}

float get_beta_coeficient() {
  float beta;
  const float T1 = 273.15;  // Temperature value 0°C converted to Kelvin.
  const float T2 = 373.15;  // Temperature value 100°C converted to Kelvin.
  const float RT1 = 27.219; // Resistance value (in ohms) at temperature T1.
  const float RT2 = 0.974;  // Resistance value (in ohms) at temperature T2.
  beta = (log(RT1 / RT2)) / ((1 / T1) - (1 / T2));  // Beta calculation.
  return beta;
}

float get_temperature() {
  float THERMISTOR_RESISTENCE = get_resistance();
  const float BETA = get_beta_coeficient();
  const double To = 298.15;    // Temperature in Kelvin for 25 degrees Celsius.
  const double Ro = RESISTANCE;   // Thermistor resistance at 25 degrees Celsius.
  double Vout = 0; // Voltage read from the thermistor's analog port.
  double Rt = 0; // Resistance read from the analog port.
  double T = 0; // Temperature in Kelvin.
  double Tc = 0; // Temperature in Celsius.

  Vout = RESISTANCE / (RESISTANCE + THERMISTOR_RESISTENCE) * VOLTAGE_UC; // Calculation of voltage read from the thermistor's analog port.
  Rt = RESISTANCE * Vout / (VOLTAGE_UC - Vout); // Calculation of resistance of the analog port.
  T = 1 / (1 / To + log(Rt / Ro) / BETA); // Application of the Beta parameter equation to obtain Temperature in Kelvin.
  Tc = T - 273.15; // Conversion from Kelvin to Celsius.
  return Tc;
}

int get_light() {
  int light_analog_value = analogRead(PHOTORESISTOR_PIN);
  int percentage = (float)(light_analog_value / 4095.0) * 100.0;
  return percentage;
}

void light_up_leds(){
  if(distance < 10){
    digitalWrite(GREEN_LED_PIN, HIGH);
  } else {
    digitalWrite(GREEN_LED_PIN, LOW);
  }

  if(temperature > 40.0){
    digitalWrite(YELLOW_LED_PIN, HIGH);
  } else {
    digitalWrite(YELLOW_LED_PIN, LOW);
  }

  if(light > 60){
    digitalWrite(RED_LED_PIN, HIGH);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
  }
}

void serial_measured_data(){
  Serial.print("Distance (in Centimeters): ");
  Serial.println(distance);
  Serial.print("Temperature (in Celsius): ");
  Serial.println(temperature);
  Serial.print("Light intensity (in Percentage): ");
  Serial.println(light);
  Serial.println("---");
}

/****************************************
 * Main Functions
 ****************************************/

void setup() {
  Serial.begin(115200);
  // ubidots.setDebug(true);  // uncomment this to make debug messages available
  ubidots.connectToWifi(WIFI_SSID, WIFI_PASS);
  ubidots.setCallback(callback);
  ubidots.setup();
  ubidots.reconnect();

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  timer = millis();
}

void loop(){
  if (!ubidots.connected()) {
    ubidots.reconnect();
  }

  if ((millis() - timer) > PUBLISH_FREQUENCY) { // Triggers the publish data routine every 10 seconds
    distance = get_distance();
    ubidots.add(HC_SR04_LABEL, distance);
    temperature = get_temperature();
    ubidots.add(THERMISTOR_NTC_LABEL, temperature);
    light = get_light();
    ubidots.add(PHOTORESISTOR_LDR_LABEL, light);
    ubidots.publish(DEVICE_LABEL);
    timer = millis();
  }
  
  light_up_leds();
  serial_measured_data();
  ubidots.loop(); // Maintain the connection active

  delay(1000); // Pause during 1 second
}