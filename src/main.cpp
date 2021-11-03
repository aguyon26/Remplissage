#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
//#include <SPIFFS.h>
#define _ESPASYNC_WIFIMGR_LOGLEVEL_    3
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>
/****************************************************************************************************************************
  Async_AutoConnect_ESP32_minimal.ino
  For ESP8266 / ESP32 boards
  Built by Khoi Hoang https://github.com/khoih-prog/ESPAsync_WiFiManager
  Licensed under MIT license
 *****************************************************************************************************************************/
#if !(defined(ESP32) )
  #error This code is intended to run on the ESP32 platform! Please check your Tools->Board setting.
#endif
#include <ESPAsync_WiFiManager.h>              //https://github.com/khoih-prog/ESPAsync_WiFiManager



//AsyncWebServer webServer(80);
#if !( USING_ESP32_S2 || USING_ESP32_C3 )
DNSServer dnsServer;
#endif
//const char *ssid = "";
//const char *password = "";

const int led = 02;
const int resetButton =18; //D5 UNO
const int capteurPression = 35;
const int capteurTop = 26;   

int valeurDelayLed = 1000;
bool etatLed = 0;
bool etatLedVoulu = 0;
int previousMillis = 0;

AsyncWebServer server(80);
//WiFiManager wm;


//----------------------------------------------------ISR
unsigned long CountInput = 0;
void IRAM_ATTR isr() {
 CountInput = CountInput+1; // Formule de calcul:F(Hz)= 109 * débit (Q)(l/min)
}



void setup()
{
   
  //----------------------------------------------------Serial
  Serial.begin(115200);
  Serial.println("\n");
  //----------------------------------------------------GPIO
  pinMode(led, OUTPUT);
  pinMode(resetButton,INPUT_PULLUP);
  pinMode(capteurTop, INPUT_PULLDOWN);

  digitalWrite(led, HIGH);
  pinMode(capteurPression, INPUT);

  //----------------------------------------------------Interrupt
  attachInterrupt(capteurTop, isr, FALLING);

  ///---------------------------------------------------Wifi
  // put your setup code here, to run once:
  Serial.print("\nStarting Async_AutoConnect_ESP32_minimal on " + String(ARDUINO_BOARD)); Serial.println(ESP_ASYNC_WIFIMANAGER_VERSION);
#if ( USING_ESP32_S2 || USING_ESP32_C3 )
  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, NULL, "Bottle_Filler_Autoconnect");
#else
  ESPAsync_WiFiManager ESPAsync_wifiManager(&server, &dnsServer, "Bottle_Filler_Autoconnect");
#endif  
  if (!digitalRead(resetButton))
  {
  Serial.println("###### Reset WIFI ############");
  ESPAsync_wifiManager.resetSettings();   //reset saved settings
  
  }
  ESPAsync_wifiManager.setAPStaticIPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  ESPAsync_wifiManager.autoConnect("Bottle_FillerAP");

  if (WiFi.status() == WL_CONNECTED) { Serial.print(F("Connected. Local IP: ")); Serial.println(WiFi.localIP()); }
  else { Serial.println(ESPAsync_wifiManager.getStatus(WiFi.status())); }
 
 ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

  //----------------------------------------------------SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("Erreur SPIFFS...");
    //return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("File: ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

 

  //----------------------------------------------------WIFI
	//WiFi.mode(WIFI_STA);
  digitalWrite(led, LOW);
  //WiFi.begin(ssid, password);
  Serial.print("Tentative de connexion...");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\n");
  Serial.println("Connexion etablie!");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  //mDNS Responder
  // Set up mDNS responder:
    // - first argument is the domain name, in this example
    //   the fully-qualified domain name is "esp32.local"
    // - second argument is the IP address to advertise
    //   we send our IP address on the WiFi network
    if (!MDNS.begin("ESP1")) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");

    Serial.println("TCP server started");

    // Add service to MDNS-SD
    MDNS.addService("http", "tcp", 80);
  //----------------------------------------------------SERVER
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/w3.css", "text/css"); });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "text/javascript"); });

  server.on("/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/jquery-3.4.1.min.js", "text/javascript"); });

  server.on("/param.xml", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
              request->send(SPIFFS, "/param.xml", "text/xml");});

  server.on("/wparamxml", HTTP_POST, [](AsyncWebServerRequest *request)
            { 
                Serial.println("wparam");
                File file = SPIFFS.open("/param.xml","w");
                String message;
                message = request->getParam("wparamxml", true)->value();
                Serial.println(message);
                file.print(message);

              request->send(204);});

  server.on("/lectureValeur1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              float sensorValue = float (map(float (analogRead(capteurPression)), 0.0,7500.0 ,0.0, 500.0))/100.0;
              //int val = analogRead(capteurPression);
              String Value = String(sensorValue);
              request->send(200, "text/plain", Value);
              //Serial.println(Value);
            });
 server.on("/lectureValeur2", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String Value = String(CountInput);
              request->send(200, "text/plain", Value);
              //Serial.println(Value);
            });
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatLedVoulu = 1;
              request->send(204);
             // Serial.println("On");
            });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatLedVoulu = 0;
              digitalWrite(led, LOW);
              etatLed = 0;
              request->send(204);
              // Serial.println("off");
            });

  server.on("/delayLed", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("valeurDelayLed", true))
              {
                String message;
                message = request->getParam("valeurDelayLed", true)->value();
                if(message.toInt()!=0)
                valeurDelayLed = message.toInt();
                Serial.println("valeurDelayLed");
              }
              request->send(204);
            });
  server.on("/valeurSaisi1", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("valeurSaisi1", true))
              {
                String message;
                message = request->getParam("valeurSaisi1", true)->value();
                if(message.toInt()!=0)
                valeurDelayLed = message.toInt();
                 Serial.println("valeurSaisi1");
              }
              request->send(204);
            });

    server.on("/valeurSaisi2", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("valeurSaisi2", true))
              {
                String message;
                message = request->getParam("valeurSaisi2", true)->value();
                if(message.toInt()!=0)
                valeurDelayLed = message.toInt();
                 //Serial.println("valeurSaisi2");
              }
              request->send(204);
            });
   server.on("/valeurSaisi3", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("valeurSaisi3", true))
              {
                String message;
                message = request->getParam("valeurSaisi3", true)->value();
                if(message.toInt()!=0)
                valeurDelayLed = message.toInt();
                // Serial.println("valeurSaisi3");
              }
              request->send(204);
            });
   server.on("/valeurSaisi4", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("valeurSaisi4", true))
              {
                String message;
                message = request->getParam("valeurSaisi4", true)->value();
                if(message.toInt()!=0)
                valeurDelayLed = message.toInt();
                // Serial.println("valeurSaisi4");
              }
              request->send(204);
            });
  //----------------------------------------------------OTA
  Serial.println("Starting OTA...");
  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("Serveur actif!");
}

void loop()
{
 
 ArduinoOTA.handle();
   if(etatLedVoulu)
  {
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= valeurDelayLed)
    {
      previousMillis = currentMillis;

      etatLed = !etatLed;
      digitalWrite(led, etatLed);
    }

 /* if(digitalRead(resetButton))
	{
		Serial.println("Suppression des reglages et redemarrage...");
		wm.resetSettings();
		ESP.restart();
	}*/

  }

/*
Gestion de la pression.
ETAPE : 0 "Purge manuel de la boueille" #DMD => OUVRIR GAZ PURGE
T0 : "Détection préssion dans la bouteille P0 (mémorisation)" et Delta P,  stable après temps"
ETAPE : 10 "Purge gaz C02" #CMD => OUVERTURE VANNE PURGE
T10 : "X secondes"
ETAPE : 20 "Attentente retour Pression Purge P0"
T20 : "Pression >= P0 (mémorisation)"
ETAPE : 30 "Passage manuel Ligquide " #DMD => OUVRIR LIQUIDE  #CMD=> OUVERTURE VANNE LIQUIDE #CMD =>RAZ COMPTEUR LIQUIDE
T30 : "Pression > P0 (mémorisation) et Delta P,  stable après temps"
ETAPE : 40 "Ouverture Vanne Purge "
T40 : "Compteur Liquide >=Seuil Compteurs"
ETAPE : 50 "Fermeture Vanne Liquide "
T50 : "Pression < proche 0,5 bar et Delta P,  stable après temps"
ETAPE : 50 "Fermeture Vanne Purge " 
T50 : "true"

RETOUR a l'étape"0"

*/

}

 