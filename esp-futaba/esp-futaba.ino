// https://qiita.com/exabugs/items/2f67ae363a1387c8967c

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <FS.h>
#include <SoftwareSerial.h>
#include "setting.h"


// モード切り替えピン
const int MODE_PIN = 0; // GPIO0

const char* settings = "/wifi_settings.txt";  // Wi-Fi設定保存ファイル
const String pass = "password";               // サーバモードでのパスワード
String host = HOST;
int port = PORT;
int id = ID;

byte trqOn[] = {0xFA, 0xAF, 0x01, 0x00, 0x24, 0x01, 0x01, 0x01, 0x24};  //トルクON





bool setupMode;
ESP8266WebServer server(80);
WiFiClient client;

SoftwareSerial SERVO(14, 12, false, 256);


/**
 * WiFi設定
 */
void handleRootGet() {
  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += "<form method='post'>";
  html += "  <input type='text' name='ssid' placeholder='ssid'><br>";
  html += "  <input type='text' name='pass' placeholder='pass'><br>";
  html += "  <input type='submit'><br>";
  html += "</form>";
  server.send(200, "text/html", html);
}

void handleRootPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  File f = SPIFFS.open(settings, "w");
  f.println(ssid);
  f.println(pass);
  f.close();

  String html = "";
  html += "<h1>WiFi Settings</h1>";
  html += ssid + "<br>";
  html += pass + "<br>";
  server.send(200, "text/html", html);
}

/**
 * 初期化(クライアントモード)
 */
void setup_client() {

//  File f = SPIFFS.open(settings, "r");
//  String ssid = f.readStringUntil('\n');
//  String pass = f.readStringUntil('\n');
//  f.close();
  String ssid = SSID;
  String pass = PASS;

  ssid.trim();
  pass.trim();

  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);

  WiFi.begin(ssid.c_str(), pass.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * 初期化(サーバモード)
 */
void setup_server() {
  String ssid = "EnkakuOshaku";
  Serial.println("SSID: " + ssid);
  Serial.println("PASS: " + pass);
  Serial.println("http://192.168.4.1/");

  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid.c_str(), pass.c_str());

  server.on("/", HTTP_GET, handleRootGet);
  server.on("/", HTTP_POST, handleRootPost);
  server.begin();
  Serial.println("HTTP server started.");
}

void Move_SV(unsigned char id, int angle) {
  unsigned char TxData[10];   //送信データバッファ [10byte]
  unsigned char CheckSum = 0; // チェックサム計算用変数
  TxData[0] = 0xFA;           // Header
  TxData[1] = 0xAF;           // Header
  TxData[2] = id;             // ID
  TxData[3] = 0x00;           // Flags
  TxData[4] = 0x1E;           // Address
  TxData[5] = 0x02;           // Length
  TxData[6] = 0x01;           // Count
  // Angle
  TxData[7] = (unsigned char)0x00FF & angle;        // Low byte
  TxData[8] = (unsigned char)0x00FF & (angle >> 8); //Hi byte
  // チェックサム計算
  for(int i=2; i<=8; i++) {
    CheckSum = CheckSum ^ TxData[i]; // ID～DATAまでのXOR
  }
  TxData[9] = CheckSum;      //Sum
  Serial.print("Move ");
  Serial.print(angle);
  Serial.print(" [");
  for(int i=0; i<=9; i++) {
    SERVO.write(TxData[i]);
    Serial.print(TxData[i], HEX);
    Serial.print(" ");
  }
  Serial.println("]");
}

/**
 * 初期化
 */
void setup() {
  Serial.begin(115200);
  SERVO.begin(115200);
  Serial.println("setup 1");

  // 1秒以内にMODEを切り替える
  //  0 : Server
  //  1 : Client
  delay(1000);
  Serial.println("setup 2");

  // ファイルシステム初期化
  SPIFFS.begin();

  pinMode(MODE_PIN, INPUT);
  if (digitalRead(MODE_PIN) == 0) {
    // サーバモード初期化
    Serial.println("setup 3");
    setup_server();
    setupMode = true;
  } else {
    // クライアントモード初期化
    Serial.println("setup 4");
    setup_client();
    setupMode = false;
  }
  SERVO.write(trqOn, 9);      // トルクON
}

void loop() {
  if (setupMode) {
    server.handleClient();
  } else {
    if (!client.connected()) {
      Serial.println("not connected");
      if (!client.connect(host, port)) {
        Serial.println("connection failed:" + host + ":" + port);
      } else {
        Serial.println("connection success:" + host + ":" + port);
      }
    }
//    uint8_t buf[] = {0xff, 1, 200};
//    delay(500);
//    client.write(buf, 3);
    if (client.available() >= 3) {
      int c1 = client.read();
      int c2 = client.read();
      int c3 = client.read();
      Serial.print(c1);
      Serial.print(" ");
      Serial.print(c2);
      Serial.print(" ");
      Serial.println(c3);
      if (c1 == 0xff && c2 == id) {
        int angle = (c3-30) * 15;
        Move_SV(1, angle);
      }
    }
    delay(50);
  }
}
