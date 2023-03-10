#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include <MPU6050_light.h>
MPU6050 mpu(Wire);
int leftMotor_speed, rightMotor_speed, servoAngle;

#include "Wire.h"
//Add libraries for your sensor here if necessary

// Wifi details and Raspberry Pi Address
const char* ssid     = "c19boys";
const char* password = "c19raspberrypi";  
const char* mqtt_server = "192.168.2.1"; 


WiFiClient espClient;
PubSubClient client(espClient);

#include <Wire.h>
#define I2C_SLAVE_ADDR 0x04 // 4 in hexadecimal

long lastMsg = 0;


void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Wire.begin();
  
  byte status = mpu.begin();
  Serial.print(F("MPU6050 status: "));
  Serial.println(status);
  while(status!=0){ } // stop everything if could not connect to MPU6050
  
  Serial.println(F("Calculating offsets, do not move MPU6050"));
  delay(1000);
  mpu.calcOffsets(); // gyro and accelero
  Serial.println("Done!\n");
}

// Connecting to Wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Getting meesage from Node Red Dashboard. Used as the car's remote contol
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Check if message has arrived from Node Red to control EEEBot
  if(String(topic) == "esp32/output2"){
    if (messageTemp == "forward"){
      // Move EEEBot Forward
      forwards();
    }
    else if (messageTemp == "backward"){
      // Move EEEBot Backward
      backwards();
    }
    else if (messageTemp == "left"){
      // Turn EEEBot Left
      turn_left();
    }
    else if (messageTemp == "right"){
      // Turn EEEBot Right
      turn_right();
    }
    else if (messageTemp == "stop"){
      // Stop EEEBot
      stop_motors();
    }
  } 
  
}

// Connect to MQTT Broker on Raspberry Pi
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      //Subscribe to esp32/output1 for remote control
      client.subscribe("esp32/output1");
         
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}


void loop() {
  mpu.update();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 200) {
    lastMsg = now;
    //Read y angle
    double y_angle = mpu.getAngleY();
    // Convert the value to a char array
    char distString[8];
    dtostrf(y_angle, 1, 2, distString);
    client.publish("esp32/gyroy", distString);
  }

}

// Send motor speed and servo angle to arduino via I2C
void transmit_to_arduino(){
  Wire.beginTransmission(I2C_SLAVE_ADDR); // transmit to device #4
    Wire.write((byte)((leftMotor_speed & 0x0000FF00) >> 8));    // first byte of x, containing bits 16 to 9
    Wire.write((byte)(leftMotor_speed & 0x000000FF));           // second byte of x, containing the 8 LSB - bits 8 to 1
    Wire.write((byte)((rightMotor_speed & 0x0000FF00) >> 8));    // first byte of y, containing bits 16 to 9
    Wire.write((byte)(rightMotor_speed & 0x000000FF));           // second byte of y, containing the 8 LSB - bits 8 to 1
    
    Wire.write((byte)((servoAngle & 0x0000FF00) >> 8));    // first byte of y, containing bits 16 to 9
    Wire.write((byte)(servoAngle & 0x000000FF));           // second byte of y, containing the 8 LSB - bits 8 to 1
  Wire.endTransmission();   // stop transmitting 
  
}

// Functions to move car
void forwards(){
  leftMotor_speed=150;
  rightMotor_speed=150;
  servoAngle=83;
  transmit_to_arduino();
}

void backwards(){
  leftMotor_speed=-150;
  rightMotor_speed=-150;
  servoAngle=83;
  transmit_to_arduino();
}

void stop_motors(){
  leftMotor_speed=0;
  rightMotor_speed=0;
  servoAngle=83;
  transmit_to_arduino();
}

void turn_left(){
  leftMotor_speed=90;
  rightMotor_speed=150;
  servoAngle=0;
  transmit_to_arduino();
}

void turn_right(){
  leftMotor_speed=150;
  rightMotor_speed=90;
  servoAngle=140;
  transmit_to_arduino();
}

void back_left(){
  leftMotor_speed=-90;
  rightMotor_speed=-150;
  servoAngle=0;
  transmit_to_arduino();
}

void back_right(){
  leftMotor_speed=-150;
  rightMotor_speed=-90;
  servoAngle=140;
  transmit_to_arduino();
}
