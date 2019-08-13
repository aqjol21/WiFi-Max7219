#include <Arduino.h>

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"

const char* ssid = "beintechhw";
const char* password = "12345678";
AsyncWebServer server(80);

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4

#define CLK_PIN   18  // or SCK
#define DATA_PIN  19  // or MOSI
#define CS_PIN    5  // or SS

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

uint8_t scrollSpeed = 40;    // default frame delay value
textEffect_t scrollEffect = PA_SCROLL_LEFT;
textPosition_t scrollAlign = PA_LEFT;
uint16_t scrollPause = 0; // in milliseconds

// Global message buffers shared by Serial and Scrolling functions
#define  BUF_SIZE  75
char curMessage[BUF_SIZE] = { "" };
char newMessage[BUF_SIZE] = { "Hello! Enter new message?" };
bool newMessageAvailable = true;

void readSerial(void)
{
  static char *cp = newMessage;

  while (Serial.available())
  {
    *cp = (char)Serial.read();
    if ((*cp == '\n') || (cp - newMessage >= BUF_SIZE-2)) // end of message character or full buffer
    {
      *cp = '\0'; // end the string
      // restart the index for next filling spree and flag we have a message waiting
      cp = newMessage;
      newMessageAvailable = true;
    }
    else  // move char pointer to next position
      cp++;
  }
}
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:">
<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css">
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js"></script>
  <script>
    function sliderChange(val){
      document.getElementById('bright').innerHTML = val;
      fetch(`?m=${val}&`);
  }
  </script>
</head>
  <body>
    <div class="container"><div class="row"><h1>MAX7219</h1></div>
        <form onsubmit="sliderChange()" >
            <label>Message:<br><input type="text" name="m" maxlength="255" />
        </form>
            </div>

  </body>
  </html>
  <!--<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:">
<link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css">
  <script src="https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js"></script>
</head>
  <body>
    <div class="container"><div class="row"><h1>MAX7219</h1></div>
      <form id="data_form" name="frmText">
      <label>Message:<br><input type="text" name="m" maxlength="255"></label> 
    </div>
  
  </body>
  </html>-->
)rawliteral";


void setup()
{
  Serial.begin(115200);
  Serial.print("\n[Parola Scrolling Display]\nType a message for the scrolling display\nEnd message line with a newline");

  P.begin();
  P.displayText(curMessage, scrollAlign, scrollSpeed, scrollPause, scrollEffect, scrollEffect);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  sprintf(newMessage, "%03d.%03d.%03d.%03d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  newMessageAvailable = true;
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
 
    int paramsNr = request->params();
    Serial.println(paramsNr);
 
    for(int i=0;i<paramsNr;i++){
 
        AsyncWebParameter* p = request->getParam(i);
        Serial.print("Param name: ");
        Serial.println(p->name());
        Serial.print("Param value: ");
        Serial.println(p->value());
        Serial.println("------");
        String prm = p->name();
        if(prm=="m"){
              String value = p->value();
              value.toCharArray(newMessage, BUF_SIZE);
              newMessageAvailable = true;
              Serial.println("Changing text");
        }
        else if(prm=="u"){
              String value = p->value();
              P.setSpeed(value.toInt());              
              Serial.println("Changing speed");
        }
        else if(prm=="s"){
              String value = p->value();
              Serial.println("Changing speed");
              if(value=="r")
                scrollEffect = PA_SCROLL_RIGHT;
              else
                scrollEffect = PA_SCROLL_LEFT;
              P.setTextEffect(scrollEffect, scrollEffect);
        }
        else if(prm=="i"){
              String value = p->value();
              if(value=="0")
                P.setInvert(false);
              else 
                P.setInvert(true);
              Serial.println("Inverting");
        }
        else if(prm=="b"){
              String value = p->value();
              P.setIntensity(value.toInt());
              Serial.println("Changing brightness");
        }
        
        
    }
 
    request->send(200, "text/plain", "message received");
  });
  server.begin();
 }

void loop()
{
 
  if (P.displayAnimate())
  {
    if (newMessageAvailable)
    {
      strcpy(curMessage, newMessage);
      newMessageAvailable = false;
    }
    P.displayReset();
  }
  readSerial();
}