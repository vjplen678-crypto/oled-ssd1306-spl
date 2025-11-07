#define TRIG_L1 9
#define ECHO_L1 8
#define TRIG_L2 2
#define ECHO_L2 3
#define TRIG_R1 6
#define ECHO_R1 7
#define TRIG_R2 5
#define ECHO_R2 4

// Ngõ ra tương ứng
#define OUT_L1 A0
#define OUT_L2 A1
#define OUT_R1 A2
#define OUT_R2 A3

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_L1, OUTPUT); pinMode(ECHO_L1, INPUT);
  pinMode(TRIG_L2, OUTPUT); pinMode(ECHO_L2, INPUT);
  pinMode(TRIG_R1, OUTPUT); pinMode(ECHO_R1, INPUT);
  pinMode(TRIG_R2, OUTPUT); pinMode(ECHO_R2, INPUT);

  pinMode(OUT_L1, OUTPUT);
  pinMode(OUT_L2, OUTPUT);
  pinMode(OUT_R1, OUTPUT);
  pinMode(OUT_R2, OUTPUT);

  Serial.println("Arduino ready");
}

float readDistance(int trig, int echo) {
  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);  // Timeout 30ms
  if (duration == 0) return 999;               // Không đo được
  return duration * 0.034 / 2;                 // cm
}

void loop() {
  float l1 = readDistance(TRIG_L1, ECHO_L1);
  float l2 = readDistance(TRIG_L2, ECHO_L2);
  float r1 = readDistance(TRIG_R1, ECHO_R1);
  float r2 = readDistance(TRIG_R2, ECHO_R2);

  // Xuất ra A0–A3 theo ngưỡng 15 cm
  digitalWrite(OUT_L1, (l1 > 15) ? HIGH : LOW);
  digitalWrite(OUT_L2, (l2 > 15) ? HIGH : LOW);
  digitalWrite(OUT_R1, (r1 > 15) ? HIGH : LOW);
  digitalWrite(OUT_R2, (r2 > 15) ? HIGH : LOW);

  // In ra debug
  Serial.print("L1="); Serial.print(l1);
  Serial.print(" | L2="); Serial.print(l2);
  Serial.print(" | R1="); Serial.print(r1);
  Serial.print(" | R2="); Serial.print(r2);
  Serial.println(" cm");


}
