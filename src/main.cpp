/*
 * Copyright (C) 2025 Bruno Pirrotta
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <regex>

AsyncWebServer server(80);
const int ssidIndex=3;
const int idespIndex=0;
const int passwordIndex=36;
const int mqtthostIndex=68;
const int mqttuserIndex=100;
const int mqttpwdIndex=132;
const int mqtthostportIndex=164;

String idesp = "";
String ssidWifi = "";
String passwordWifi = "";
String mqtthost = "";
String mqtthostport = "";
String mqttuser ="";
String mqttpwd = "";

const IPAddress mqtt_server(192, 168, 0, 14);
const char* mqttUser     = "bruno";
const char* mqttPassword = "bruno";
byte scrState = 0;

//const char* mqtt_server = "192.168.0.14";

// these pins are used to connect to the SCR
#define PIN_ZERO 2
#define PIN_SCR 3 
#define BLUE_LED 1


WiFiClient espClient;
PubSubClient client(espClient);
String outTopic;
String inTopic;
String debugTopic;

unsigned long now;
unsigned long changeTime;

boolean isUp =0 ;
// current power (as a percentage of time) : power off at startup.
float power = 0;

const char* ssid    = "ESP8266-Access-Point";
const char* password = "123456789";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      h2 {
        font-size: 3.0rem;
      }
      p { 
        font-size: 3.0rem;
      }
      .units {
        font-size: 1.2rem;
      }
      .input {
        font-size: 20px;
        margin-bottom: 10px;
      }
      .wifi-form {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: space-around;
      }
      #sentLabel {
        display: none;
        margin-bottom: 10px;
        font-weight: bold;
      }
    </style>
  </head>
  <body onload="getdata()">
    <div class="wifi-form">
      <h1>Configuration Module de Puissance</h1>
      <label for="idesp">ID EQUIPEMENT</label>
      <input id="idesp" class="input" type="number" maxlength="1">
      <label for="ssid">WIFI ID (SSID)</label>
      <input id="ssid" class="input" type="text" maxlength="32">
      <label for="password">WIFI PASSWORD</label>
      <input id="password" class="input" type="password">
      <label for="mqtthost">MQTT HOST IP</label>
      <input id="mqtthost" class="input" type="text">
      <label for="mqtthostport">MQTT HOST PORT</label>
      <input id="mqtthostport" class="input" type="text" value="1883">
      <label for="mqttuser">MQTT USER</label>
      <input id="mqttuser" class="input" type="text">
      <label for="mqttpwd">MQTT PASSWORD</label>
      <input id="mqttpwd" class="input" type="password">
      <button onclick="connectToWifi()">Save configuration</button>
      <label id="sentLabel">Informations de configuration sauvegardée</label>
    </div>
  </body>
  <script>
    function getdata() {
      fetch('/getdata')
      .then(response => response.text())
      .then(data => { let array = data.split("%");
              document.getElementById('idesp').value = array[0] || "";
              document.getElementById('ssid').value = array[1] || "";
              document.getElementById('password').value = array[2] || "";
              document.getElementById('mqtthost').value = array[3] || "";
              document.getElementById('mqtthostport').value = array[4] || "";
              document.getElementById('mqttuser').value = array[5] || "";
              document.getElementById('mqttpwd').value = array[6] || "";
              });
    }

    function connectToWifi() {
      var idesp = document.getElementById("idesp").value;
      var ssid = document.getElementById("ssid").value;
      var password = document.getElementById("password").value;
      var mqtthost = document.getElementById("mqtthost").value;
      var mqtthostport = document.getElementById("mqtthostport").value;
      var mqttuser = document.getElementById("mqttuser").value;
      var mqttpwd = document.getElementById("mqttpwd").value;
      var xhr = new XMLHttpRequest(); xhr.open("POST", "/", true);
      xhr.setRequestHeader('Content-Type', 'application/json');
      xhr.send(idesp + "%" + ssid + "%" + password + "%" + mqtthost + "%" + mqtthostport + "%" + mqttuser + "%" + mqttpwd);
      document.getElementById("sentLabel").style.display = "inline";
    }
  </script>
</html>)rawliteral";



void writeEEPROM(String value, int address) {
  int len = value.length();
  EEPROM.put(address, len);
  for (int i = address + 1; i < len + address + 1; ++i) {
    EEPROM.put(i, value[i - address - 1]);
  }
  EEPROM.commit();
}

String readEEPROM(int address) {
  uint8_t len = EEPROM.read(address);
  if (len == 255) return "";
  String res;
  for (int i = address + 1; i < len + address + 1; ++i) {
    char c = (char)EEPROM.read(i);
    res.concat(c);
  }
  return res;
}
void createAccessPoint() {
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/getdata", HTTP_GET, [](AsyncWebServerRequest *request) {
    String separator ="%";
    String data = idesp + separator +ssidWifi+separator+passwordWifi+separator+ mqtthost + separator+ mqtthostport + separator+ mqttuser + separator + mqttpwd+separator;
    request->send_P(200, "text/plain", data.c_str());
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){},NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    idesp = "";
    ssidWifi = "";
    passwordWifi = "";
    mqtthost ="";
    mqtthostport = "";
    mqttuser ="";
    mqttpwd = "";

    String res((char *)data);
    int index1 = res.indexOf('%');
    int index2 = res.indexOf('%', index1 + 1);
    int index3 = res.indexOf('%', index2 + 1);
    int index4 = res.indexOf('%', index3 + 1);
    int index5 = res.indexOf('%', index4 + 1);
    int index6 = res.indexOf('%', index5 + 1);
    
    idesp = res.substring(0, index1);
    ssidWifi = res.substring(index1 +1,index2);
    passwordWifi = res.substring(index2 +1, index3);
    mqtthost = res.substring(index3 +1 , index4);
    mqtthostport = res.substring(index4 +1 , index5);
    mqttuser = res.substring(index5+1, index6);
    mqttpwd = res.substring(index6+1);
    
    writeEEPROM(idesp, idespIndex);
    writeEEPROM(ssidWifi, ssidIndex);
    writeEEPROM(passwordWifi, passwordIndex);
    writeEEPROM(mqtthost, mqtthostIndex);
    writeEEPROM(mqtthostport, mqtthostportIndex);
    writeEEPROM(mqttuser, mqttuserIndex);
    writeEEPROM(mqttpwd, mqttpwdIndex);
    
    request->send(200, "text/plain", "SUCCESS");
  });
  server.begin();
}
// this function pointer is used to store the next timer action (see call_later and onTimerISR below)
void (*timer_callback)(void);

// used to acknowledge the current power command
void sendCurrentPower() {
    client.publish(outTopic.c_str(), String(power).c_str(), true);
}
void debug(String message) {
    client.publish(debugTopic.c_str(), message.c_str(), true);
}
unsigned long previousBlinkMillis = 0;
unsigned long previousCheckMillis = 0;
const long checkInterval = 120000; // Vérifier le Wi-Fi toutes les 2 minutes (l access point restera accessible 2 min en cas de pb de connexion, avt de retenter la connexion)
bool ledState = LOW;
bool isAPMode = false;
bool wifiLost = false; // Indique si le Wi-Fi a été perdu

void readParameters() {
      ssidWifi = readEEPROM(ssidIndex);
      passwordWifi = readEEPROM(passwordIndex);
      idesp = readEEPROM(idespIndex);
      mqtthost = readEEPROM(mqtthostIndex);
      mqtthostport= readEEPROM(mqtthostportIndex);
      mqttuser =  readEEPROM(mqttuserIndex);
      mqttpwd = readEEPROM(mqttpwdIndex);
}

void setup_wifi() {
  if (ssidWifi.length() > 0 && passwordWifi.length() > 0 && WiFi.status() != WL_CONNECTED) {
    Serial.println("Tentative de connexion au Wi-Fi...");
    digitalWrite(LED_BUILTIN, LOW); // Allumer la LED pendant la connexion
    if (isAPMode) {
       WiFi.softAPdisconnect(true);
       server.end();
       isAPMode = false;
    }
    WiFi.begin(ssidWifi, passwordWifi);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      digitalWrite(LED_BUILTIN, LOW); // Éteint
      delay(100);
      digitalWrite(LED_BUILTIN, HIGH); // Allume
      delay(600);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnexion Wi-Fi réussie !");
      Serial.print("Adresse IP : ");
      Serial.println(WiFi.localIP());
      wifiLost=false;
    } 
    else {
      Serial.println("\nÉchec de connexion, passage en mode point d'accès...");
      wifiLost=true;
      isAPMode = true;
      createAccessPoint();
    }
  } else {
    Serial.println("Aucune information Wi-Fi trouvée, passage en mode point d'accès...");
    wifiLost=true;
    isAPMode = true;
    createAccessPoint();
  }
}

void on_message(char* topic, byte* payload, unsigned int length) {

    char buffer[length+0];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    float p = String(buffer).toFloat();
    if(p >= 0 && p<=100) {
        power = p;
        sendCurrentPower();
    }
}

void reconnect() {
    if (!client.connected()) {

        String clientId = "scr-";
        clientId += String(idesp);
        if (client.connect(clientId.c_str(),mqttUser,mqttPassword)) {
            client.subscribe(inTopic.c_str());
            sendCurrentPower();
        } else {
            delay(5000);
        }
    }
}
// pin ZERO interrupt routine
void IRAM_ATTR onZero() {
    changeTime = micros();
}

void setup() {
    Serial.begin(115200);
    EEPROM.begin(256);
 
    // usual pin configuration
    pinMode(PIN_SCR, OUTPUT);
    pinMode(LED_BUILTIN,OUTPUT);
    digitalWrite(PIN_SCR, LOW);
    pinMode(PIN_ZERO, INPUT);

    readParameters();    
    setup_wifi();
    // setup the MQTT stuff (one topic for commands, another one to broadcast the current power)
    outTopic = "scr/" + String(idesp) +"/out";
    inTopic = "scr/" + String(idesp) +"/in";
    debugTopic = "scr/debug/" + String(idesp);
    Serial.println("in topic: " + inTopic);
    Serial.println("out topic: " + outTopic);

    client.setServer(mqtt_server, 1883);
    client.setCallback(on_message);
    reconnect();
    Serial.println("client Connected: " + String(client.connected()));

    // setup the timer used to manage SCR tops
    // listen for change on the pin ZERO
    attachInterrupt(digitalPinToInterrupt(PIN_ZERO), onZero, CHANGE);
    
}

void blinkLED(long unsigned interval) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousBlinkMillis >= interval) {
    previousBlinkMillis = currentMillis;
    ledState = !ledState;  
    digitalWrite(LED_BUILTIN, ledState);
  }
}

// Vérification non bloquante du Wi-Fi
void checkWifi() {

  if (WiFi.status() != WL_CONNECTED) {
    if (!wifiLost) {
      Serial.println("Wi-Fi perdu, tentative de reconnexion...");
      wifiLost = true;
      WiFi.disconnect();
      readParameters();
      setup_wifi();
    }
  } else {
    if (wifiLost) {
      Serial.println("Wi-Fi reconnecté !");
      Serial.println(WiFi.localIP());
      wifiLost = false;
    }
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // Vérifier le Wi-Fi en tâche de fond
  if (currentMillis - previousCheckMillis >= checkInterval) {
    previousCheckMillis = currentMillis;
    checkWifi();
  }

  // Gestion du clignotement sans perturber le temps réel
  if (isAPMode) {
    blinkLED(100);
  }
  else if (!wifiLost) {
    blinkLED(500);
  } 
  else {
    digitalWrite(LED_BUILTIN, LOW);
  }

    if (!client.connected()) {
        reconnect();
    }
    else 
    {
    client.loop();  
    now = micros();
    unsigned int delayWait = 30+(100-power)*95;
    //debug(String(delayWait));
    if (power > 0 && now - changeTime >= delayWait && now - changeTime < 10000) {
         digitalWrite(PIN_SCR,HIGH);
         unsigned int delayOn = 50; //power < 50 ? 50 : 200;
         
         delayMicroseconds(delayOn);
    }
    }
    digitalWrite(PIN_SCR,LOW);
}