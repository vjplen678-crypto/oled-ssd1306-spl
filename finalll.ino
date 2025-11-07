#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

// ==== UART sang Arduino (TX = GPIO17, RX = GPIO16) ====
HardwareSerial mySerial(1);

// ==== Khai báo chân động cơ ====
#define enL1 13
#define inL1 14
#define inL2 12

#define enL2 25
#define inL3 26
#define inL4 27

#define enR1 4
#define inR1 5
#define inR2 18

#define enR2 22
#define inR3 21
#define inR4 19

// ==== WiFi AP ====
const char* ssid = "auto_park";
const char* password = "12345678";

WebServer server(80);

// ==== Trạng thái ====
bool mode1Active = false;
bool mode2Active = false;
bool parking = false;
bool finding = false;
unsigned long parkStart = 0;

// ==== Hàm điều khiển cơ bản (PWM) ====
void motor_forward_pwm(int en, int in1, int in2, int speed) {
  analogWrite(en, speed);
  digitalWrite(in1, HIGH);
  digitalWrite(in2, LOW);
}
void motor_back_pwm(int en, int in1, int in2, int speed) {
  analogWrite(en, speed);
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
}
void motor_stop_pwm(int en, int in1, int in2) {
  analogWrite(en, 0);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
}

// ==== Chuyển động tổng hợp ====
void car_forward_pwm(int speed) {
  motor_forward_pwm(enL1, inL1, inL2, speed);
  motor_forward_pwm(enL2, inL3, inL4, speed);
  motor_forward_pwm(enR1, inR1, inR2, speed);
  motor_forward_pwm(enR2, inR3, inR4, speed);
}
void car_left(int speed){
  motor_back_pwm(enL1, inL1, inL2, speed);
  motor_back_pwm(enL2, inL3, inL4, speed);
  motor_forward_pwm(enR1, inR1, inR2, speed);
  motor_forward_pwm(enR2, inR3, inR4, speed);
}
void car_right(int speed){
  motor_forward_pwm(enL1, inL1, inL2, speed);
  motor_forward_pwm(enL2, inL3, inL4, speed);
  motor_back_pwm(enR1, inR1, inR2, speed);
  motor_back_pwm(enR2, inR3, inR4, speed);
}
void car_back_pwm(int speed) {
  motor_back_pwm(enL1, inL1, inL2, speed);
  motor_back_pwm(enL2, inL3, inL4, speed);
  motor_back_pwm(enR1, inR1, inR2, speed);
  motor_back_pwm(enR2, inR3, inR4, speed);
}
void car_stop() {
  motor_stop_pwm(enL1, inL1, inL2);
  motor_stop_pwm(enL2, inL3, inL4);
  motor_stop_pwm(enR1, inR1, inR2);
  motor_stop_pwm(enR2, inR3, inR4);
}

// ==== Rê ngang (Mecanum) ====
void car_superleft_pwm(int speed) {
  motor_forward_pwm(enR1, inR1, inR2, speed);
  motor_back_pwm(enR2, inR3, inR4, speed);
  motor_forward_pwm(enL1, inL1, inL2, speed);
  motor_back_pwm(enL2, inL3, inL4, speed);
}
void car_superright_pwm(int speed) {
  motor_back_pwm(enL1, inL1, inL2, speed);
  motor_forward_pwm(enL2, inL3, inL4, speed);
  motor_back_pwm(enR1, inR1, inR2, speed);
  motor_forward_pwm(enR2, inR3, inR4, speed);
}

// ==== Giao diện Web ====
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html><html><head>
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>ESP32 Car</title>
  <style>
    body{font-family:sans-serif;text-align:center;background:#f4f4f4}
    button{width:120px;height:52px;margin:6px;border-radius:8px;
            border:none;background:#007BFF;color:#fff;font-size:16px}
  </style>
  <script>
    function send(cmd){fetch('/'+cmd);}
    function hold(id,cmd){
      const b=document.getElementById(id);
      b.onmousedown=()=>send(cmd);
      b.onmouseup=()=>send('S');
      b.ontouchstart=()=>send(cmd);
      b.ontouchend=()=>send('S');
    }
    window.onload=()=>{hold('f','F');hold('b','B');hold('l','L');hold('r','R');}
  </script>
  </head><body>
  <h2>ESP32 Auto Parking</h2>
  <button id='f'>Forward</button><br>
  <button id='l'>Left</button><button id='r'>Right</button><br>
  <button id='b'>Backward</button><br><br>
  <button onclick="send('S')" style="background:#dc3545">STOP</button><br><br>
  <button onclick="send('M1')" style="background:#28a745">Mode 1</button>
  <button onclick="send('M2')" style="background:#17a2b8">Mode 2</button>
  </body></html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// ==== Các route ====
void forward() { car_forward_pwm(180); server.send(200, "text/plain", "F"); }
void backward(){ car_back_pwm(180); server.send(200, "text/plain", "B"); }
void left()    { car_left(180); server.send(200, "text/plain", "L"); }
void right()   { car_right(180); server.send(200, "text/plain", "R"); }
void stopCar() { car_stop(); server.send(200, "text/plain", "S"); }

void mode1() {
  mode1Active = true;
  mode2Active = false;
  finding = true;
  parking = false;
  server.send(200,"text/plain","Mode1 Auto");
}
void mode2() {
  mode1Active = false;
  mode2Active = true;
  finding = false;
  parking = false;
  car_stop();
  server.send(200,"text/plain","Mode2 Auto Park");
}

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  pinMode(enL1, OUTPUT); pinMode(inL1, OUTPUT); pinMode(inL2, OUTPUT);
  pinMode(enL2, OUTPUT); pinMode(inL3, OUTPUT); pinMode(inL4, OUTPUT);
  pinMode(enR1, OUTPUT); pinMode(inR1, OUTPUT); pinMode(inR2, OUTPUT);
  pinMode(enR2, OUTPUT); pinMode(inR3, OUTPUT); pinMode(inR4, OUTPUT);

  car_stop();

  pinMode(33,INPUT_PULLDOWN);
  pinMode(32,INPUT_PULLDOWN);
  pinMode(35,INPUT_PULLDOWN);
  pinMode(34,INPUT_PULLDOWN);

  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/F", forward);
  server.on("/B", backward);
  server.on("/L", left);
  server.on("/R", right);
  server.on("/S", stopCar);
  server.on("/M1", mode1);
  server.on("/M2", mode2);
  server.begin();

  Serial.println("ESP32 ready. AP IP:");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();

  // ==== MODE 1: TỰ TÌM CHỖ ====
  if (mode1Active) {
    int L1= digitalRead(33);
    int L2 =digitalRead(32);
    int R3 = digitalRead(35);
    int R4 = digitalRead(34);
    Serial.printf("   L1: %d ,  L2: %d ,  R3: %d , R4: %d\n",L1,L2,R3,R4);
    
    while (L1==0 || R3==0) {
      L1= digitalRead(33);
      R3 = digitalRead(35);
      car_forward_pwm(100);
      Serial.println("Tôi đang tìm chỗ đỗ...");
    }
    car_stop();
    delay(400);
    car_superright_pwm(150);
    delay(500);
    car_stop();
    mode1Active = false;
  }

  // ==== MODE 2: TỰ ĐỖ XE ====
  if (mode2Active) {
    int L1= digitalRead(33);
    int L2 =digitalRead(32);
    int R3 = digitalRead(35);
    int R4 = digitalRead(34);
    Serial.printf("   L1: %d ,  L2: %d ,  R3: %d , R4: %d\n",L1,L2,R3,R4);
      while (L1==0 || R3==0) {
      L1= digitalRead(33);
      R3= digitalRead(35);
      car_forward_pwm(100);
      Serial.println("Tôi đang tìm chỗ đỗ...");
    }
    car_stop();
    delay(400);
    car_left(145);       // rẽ trái
    delay(600);
    car_back_pwm(90);   // lùi vào chỗ
    delay(800);
    car_stop();          // dừng
    Serial.println("Đã đỗ xe xong!");
    mode2Active = false;
  }
}
