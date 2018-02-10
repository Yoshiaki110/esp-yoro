// https://qiita.com/exabugs/items/2f67ae363a1387c8967c

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <FS.h>
#include "setting.h"


// モード切り替えピン
const int MODE_PIN = 0; // GPIO0

const char* settings = "/wifi_settings.txt";  // Wi-Fi設定保存ファイル
const String pass = "password";               // サーバモードでのパスワード
String host = HOST;
int port = PORT;
int id = ID;

bool setupMode;
ESP8266WebServer server(80);
WiFiClient client;

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

/**
 * 初期化
 */
void setup() {
  Serial.begin(115200);
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
        ;
      }
    }
    delay(50);
  }
}
