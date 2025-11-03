#include <WiFi.h>
#include <WebServer.h>

// ==== MOTOR PINS ====
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

// ==== WIFI ====
const char* ssid = "auto_park";
const char* password = "12345678";

WebServer server(80);

// ==== AUTO PARK STATES ====
enum AutoParkState { IDLE, SEARCH, ALIGN, PARKING, DONE };
AutoParkState parkState = IDLE;
String parkSide = "";
unsigned long lastCheck = 0;
unsigned long stateStart = 0;

// ==== FLAGS ====
bool mode1Active = false;

// ==== SPEED SETTINGS ====
const int SPEED_SEARCH = 90;   // tốc độ khi dò chỗ trống
const int SPEED_PARK = 80;     // tốc độ rê trái khi đỗ
const int SLIDE_DURATION = 1000; // thời gian rê trái (ms)

// ===== FUNCTION PROTOTYPES =====
void car_forward(int spd);
void car_back(int spd);
void car_left(int spd);
void car_right(int spd);
void car_stop();
void setup_motor(int en, int in1, int in2);

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17); // RX2=16, TX2=17

  setup_motor(enL1, inL1, inL2);
  setup_motor(enL2, inL3, inL4);
  setup_motor(enR1, inR1, inR2);
  setup_motor(enR2, inR3, inR4);

  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP started");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/F", forward);
  server.on("/B", backward);
  server.on("/L", left);
  server.on("/R", right);
  server.on("/S", stopCar);
  server.on("/M1", mode1);
  server.on("/M2", mode2);
  server.begin();

  Serial.println("HTTP Server started");
}

// ===== MOTOR CONTROL =====
void setup_motor(int en, int in1, int in2) {
  pinMode(en, OUTPUT); pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  ledcAttach(en, 1000, 8);
}
void motor_forward(int en, int in1, int in2, int speed){ ledcWrite(en, speed); digitalWrite(in1,HIGH); digitalWrite(in2,LOW); }
void motor_back(int en, int in1, int in2, int speed){ ledcWrite(en, speed); digitalWrite(in1,LOW); digitalWrite(in2,HIGH); }
void motor_stop(int en, int in1, int in2){ ledcWrite(en,0); digitalWrite(in1,LOW); digitalWrite(in2,LOW); }

void car_forward(int spd){
  motor_forward(enL1,inL1,inL2,spd);
  motor_forward(enL2,inL3,inL4,spd);
  motor_forward(enR1,inR1,inR2,spd);
  motor_forward(enR2,inR3,inR4,spd);
}
void car_back(int spd){
  motor_back(enL1,inL1,inL2,spd);
  motor_back(enL2,inL3,inL4,spd);
  motor_back(enR1,inR1,inR2,spd);
  motor_back(enR2,inR3,inR4,spd);
}
void car_stop(){
  motor_stop(enL1,inL1,inL2);
  motor_stop(enL2,inL3,inL4);
  motor_stop(enR1,inR1,inR2);
  motor_stop(enR2,inR3,inR4);
}
void car_left(int spd){
  motor_forward(enR1,inR1,inR2,spd);
  motor_forward(enR2,inR3,inR4,spd);
  motor_back(enL1,inL1,inL2,spd);
  motor_back(enL2,inL3,inL4,spd);
}
void car_right(int spd){
  motor_forward(enL1,inL1,inL2,spd);
  motor_forward(enL2,inL3,inL4,spd);
  motor_back(enR1,inR1,inR2,spd);
  motor_back(enR2,inR3,inR4,spd);
}

// ====== LOOP ======
void loop() {
  server.handleClient();

  if (!mode1Active) return;

  // đọc dữ liệu từ Arduino
  if (millis() - lastCheck > 200) {
    lastCheck = millis();

    if (Serial2.available()) {
      String data = Serial2.readStringUntil('\n');
      data.trim();
      if (data.length() > 0) handleSensorData(data);
    }
  }

  autoParkTask();
}

// ====== AUTO PARK LOGIC ======
void handleSensorData(String data) {
  Serial.println("[UART] " + data);

  if (parkState == SEARCH) {
    // chỉ quan tâm bên trái
    if (data.indexOf("FREE_LEFT") >= 0) {
      parkSide = "LEFT";
      parkState = ALIGN;
      stateStart = millis();
      Serial.println("Found FREE space on LEFT -> stopping...");
    }
  }
}

void autoParkTask() {
  switch (parkState) {
    case IDLE:
      break;

    case SEARCH:
      Serial.println("Searching for space (Mode 1 active)...");
      car_forward(SPEED_SEARCH);
      break;

    case ALIGN:
      car_stop();
      Serial.println("Stopping before parking...");
      delay(500);
      parkState = PARKING;
      stateStart = millis();
      break;

    case PARKING:
      Serial.println("Sliding left into space...");
      car_left(SPEED_PARK);
      if (millis() - stateStart > SLIDE_DURATION) {
        car_stop();
        parkState = DONE;
        Serial.println("✅ Parking complete!");
      }
      break;

    case DONE:
      car_stop();
      mode1Active = false;
      parkState = IDLE;
      Serial.println("Auto park finished.");
      break;
  }
}

// ====== WEB HANDLERS ======
void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html><html>
  <head>
    <title>ESP32 Auto Park</title>
    <meta name="viewport" content="width=device-width,initial-scale=1.0">
    <style>
      body{text-align:center;font-family:sans-serif;background:#f4f4f4;}
      button{width:120px;height:55px;margin:8px;border:none;border-radius:10px;font-size:16px;color:white;}
      .blue{background:#007BFF;} .red{background:#dc3545;} .green{background:#28a745;}
    </style>
    <script>
      function send(cmd){fetch("/"+cmd);}
      window.onload=()=>{['F','B','L','R'].forEach(x=>{
        const b=document.getElementById(x);
        b.addEventListener('mousedown',()=>send(x));
        b.addEventListener('mouseup',()=>send('S'));
      })};
    </script>
  </head>
  <body>
    <h2>🚗 ESP32 Car Control</h2>
    <button id="F" class="blue">Forward</button><br>
    <button id="L" class="blue">Left</button><button id="R" class="blue">Right</button><br>
    <button id="B" class="blue">Backward</button><br><br>
    <button onclick="send('S')" class="red">STOP</button><br><br>
    <button onclick="send('M1')" class="green">Mode 1: Auto Park</button>
    <button onclick="send('M2')" class="green">Mode 2: Manual</button>
  </body></html>)rawliteral";
  server.send(200, "text/html", html);
}

// ====== COMMANDS ======
void forward()  { car_forward(200); server.send(200,"text/plain","F"); }
void backward() { car_back(200);    server.send(200,"text/plain","B"); }
void left()     { car_left(200);    server.send(200,"text/plain","L"); }
void right()    { car_right(200);   server.send(200,"text/plain","R"); }
void stopCar()  { car_stop();       server.send(200,"text/plain","S"); }

void mode1(){
  mode1Active = true;
  parkState = SEARCH;
  parkSide = "";
  Serial.println("MODE 1: Auto Park STARTED");
  server.send(200, "text/plain", "Auto Park Started");
}

void mode2(){
  mode1Active = false;
  parkState = IDLE;
  car_stop();
  Serial.println("MODE 2: Manual Control");
  server.send(200, "text/plain", "Manual Mode");
}
