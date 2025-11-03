#include <SoftwareSerial.h>

// UART sang ESP32 (TX = D10, RX = D11)
SoftwareSerial mySerial(10, 11);

#define TRIG_L1 9
#define ECHO_L1 8
#define TRIG_L2 2
#define ECHO_L2 3
#define TRIG_R1 6
#define ECHO_R1 7
#define TRIG_R2 5
#define ECHO_R2 4

#define MIN_SPACE 15   // cm — khoảng cách đủ trống

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  pinMode(TRIG_L1, OUTPUT); pinMode(ECHO_L1, INPUT);
  pinMode(TRIG_L2, OUTPUT); pinMode(ECHO_L2, INPUT);
  pinMode(TRIG_R1, OUTPUT); pinMode(ECHO_R1, INPUT);
  pinMode(TRIG_R2, OUTPUT); pinMode(ECHO_R2, INPUT);
}

float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void loop() {
  float L1 = readDistance(TRIG_L1, ECHO_L1);
  float L2 = readDistance(TRIG_L2, ECHO_L2);

  bool leftFree = (L1 > MIN_SPACE && L2 > MIN_SPACE);

  if (leftFree) {
    Serial.println("FREE_LEFT");
    mySerial.println("FREE_LEFT");
  } else {
    Serial.println("BLOCK");
    mySerial.println("BLOCK");
  }

  delay(200);
}
