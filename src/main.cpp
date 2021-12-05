#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
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

const int led = 02; // AN0 UNO
const int resetButton =18; //D13 UNO
const int capteurPression = 35; // AN2 UNO
const int capteurTop = 26;   // D2 UNO

const int relay1Beer = 12;
bool etatRelay1BeerVoulu =0;
const int relay2Gaz  = 13;
bool etatRelay2GazVoulu =0;
const int relay3     = 5;
bool etatRelay3Voulu =0;
const int relay4Pump = 23;
bool etatrelay4PumpVoulu =0;

int state;
String stateInfo;
int valPres;

int deltaPres;
int timePres;
int cntRemplissage;
float sensorPres;
float valPresP0;

int valeurDelayLed = 1000;
//bool etatLed = 0;
bool etatVoulu = 0;
int previousMillis = 0;

AsyncWebServer server(80);
//WiFiManager wm;


//----------------------------------------------------ISR
unsigned long CountInput = 0;
void IRAM_ATTR isr() {
 CountInput = CountInput+1; // Formule de calcul:F(Hz)= 109 * débit (Q)(l/min)
}

//----------------------------------------------------Millis
unsigned long long superMillis() {
  static unsigned long nbRollover = 0;
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  
  if (currentMillis < previousMillis) {
     nbRollover++;
  }
  previousMillis = currentMillis;

  unsigned long long finalMillis = nbRollover;
  finalMillis <<= 32;
  finalMillis +=  currentMillis;
  return finalMillis;
}


void setup()
{
   
  //----------------------------------------------------Serial
  Serial.begin(115200);
  Serial.println("\n");
  //----------------------------------------------------GPIO
  pinMode(led, OUTPUT);
  pinMode(relay1Beer, OUTPUT);
  pinMode(relay2Gaz, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4Pump, OUTPUT);
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
  Serial.println("############ Reset WIFI ############");
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
              sensorPres = float (map(float (analogRead(capteurPression)), 0.0,7500.0 ,0.0, 500.0))/100.0;
              String Value = String(sensorPres);
              request->send(200, "text/plain", Value);
            });
 server.on("/lectureValeur2", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String Value = String(CountInput);
              request->send(200, "text/plain", Value);
            });
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatVoulu = 1;
              request->send(204);
            });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatVoulu = 0;
              
              request->send(204);
            });
  server.on("/on1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatrelay4PumpVoulu = 1;
              request->send(204);
            });

  server.on("/off1", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              etatrelay4PumpVoulu = 0;
              request->send(204);
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
                valPres = message.toInt();
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
                deltaPres = message.toInt();
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
                timePres = message.toInt();
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
                cntRemplissage = message.toInt();
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
 unsigned long currentMillis = superMillis();

   if(etatVoulu)
  {
      switch (state)
      {
      case 0 : /* E0 : Purge manuel  */
        stateInfo = "Purge manuel de la bouteille";
        etatRelay1BeerVoulu =0;
        etatRelay2GazVoulu =0;
        if( sensorPres > float(valPres))
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =0;
            etatRelay2GazVoulu =0;
            CountInput =0;       
            state = 10;
            valPresP0  = sensorPres; 
        }
      break;

      case 10 : /* E10 : Purge gaz C02  */
        stateInfo = "Purge gaz C02 1,5 secondes";
        etatRelay1BeerVoulu =0;
        etatRelay2GazVoulu =1;
        if( sensorPres < float(valPres) && (currentMillis-previousMillis)>=1500)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =0;
            etatRelay2GazVoulu =0;
            CountInput =0;
            state = 20;
        }
      break;

      case 20 : /* E20 : Attentente retour Pression Purge P0 */
        stateInfo = "Attentente retour Pression Purge P0";
        etatRelay1BeerVoulu =0;
        etatRelay2GazVoulu =0;
        if( sensorPres > valPresP0 && (currentMillis-previousMillis)>=1500)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =0;
            etatRelay2GazVoulu =0;
            state = 30;
            CountInput =0;
        }
      break;

      case 30 : /* E30 : Passage manuel Ligquide */
        stateInfo = "Passage manuel Ligquide";
        etatRelay1BeerVoulu =1;
        etatRelay2GazVoulu =0;
        if(0 <= CountInput && sensorPres > float(valPres+deltaPres) && (currentMillis-previousMillis)>=1500)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =1;
            etatRelay2GazVoulu =0;
            state = 40;
        }
      break;

      case 40 : /* E40 : Pression stable + Ligquide */
        stateInfo = "Pression stable + Ligquide ";
        etatRelay1BeerVoulu =1;
        etatRelay2GazVoulu =0;
        if(0 <= cntRemplissage && sensorPres > float(valPres+deltaPres) && (currentMillis-previousMillis)>=timePres)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =1;
            etatRelay2GazVoulu =0;
            state = 50;
        }
      break;

      case 50 : /* E50 : Ouverture Vanne Purge */
        stateInfo = "Ouverture Vanne Purge";
        etatRelay1BeerVoulu =1;
        etatRelay2GazVoulu =1;
        if(CountInput>= cntRemplissage)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =0;
            etatRelay2GazVoulu =0;
            state = 60;
        }
      break;

      case 60 : /* E60 : Ouverture Vanne Purge */
        stateInfo = "Ouverture Vanne Purge";
        etatRelay1BeerVoulu =0;
        etatRelay2GazVoulu =1;
        if(sensorPres < float(5) && (currentMillis-previousMillis)>=1500)
        {
            previousMillis = currentMillis; 
            etatRelay1BeerVoulu =0;
            etatRelay2GazVoulu =1;
            state = 00;
        }
      break;

      default:
        break;

      }
  }else
      state = 0;

  digitalWrite(led, etatVoulu);
  digitalWrite(relay1Beer,etatRelay1BeerVoulu);
  digitalWrite(relay2Gaz,etatRelay2GazVoulu);
  digitalWrite(relay3,etatRelay3Voulu);
  digitalWrite(relay4Pump,etatrelay4PumpVoulu);
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



    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= valeurDelayLed)
    {
      previousMillis = currentMillis;

      etatLed = !etatLed;
      digitalWrite(led, etatLed);
    }
*/

}


