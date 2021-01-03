#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Servo.h>
#include <LittleFS.h>

#include "secrets.h"

const char *ssid = SECRET_SSID;
const char *password = SECRET_PASSWORD;
const uint8_t servoPin = D2;
Servo servo;
AsyncWebServer server(80);

// Predefined servo angles
enum ServoAngle {
    dropLeft = 70,
    center = 90,
    dropRight = 110
};

ServoAngle currentStatus = center;

std::string buildHTMLString()
{    
    std::string htmlStatus = 
        "<!DOCTYPE html>"
        "<html lang=\"en\">"
        "<head>"
        "    <meta charset=\"UTF-8\">"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <link href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh\" crossorigin=\"anonymous\">"
        "    <title>Drone Dropper</title>"
        "</head>"
        "<style>"
        "</style>"
        "<body>"
        "    <div class=\"container jumbotron text-center\">"
        "    <h1 class=\"display-4\">Drone Dropper</h1><br>";

    htmlStatus += 
        "    <a class=\"btn btn-primary ";
    if (currentStatus == dropLeft) { htmlStatus += "disabled"; }
    htmlStatus += "\" href=\"/dropleft\" role=\"button\">Drop Left</a>";

    htmlStatus += 
        "    <a class=\"btn btn-primary ";
    if (currentStatus == center) { htmlStatus += "disabled"; }
    htmlStatus += "\" href=\"/center\" role=\"button\">Center</a>";

    htmlStatus += 
        "    <a class=\"btn btn-primary ";
    if (currentStatus == dropRight) { htmlStatus += "disabled"; }
    htmlStatus += "\" href=\"/dropright\" role=\"button\">Drop Right</a>";

    htmlStatus +=
        "    <br /><br />"
        "    🔋 <span class=\"badge badge-warning\">45%</span>"
        "    </div>"
        "    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.4.1/jquery.min.js\"></script>"
        "    <script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js\" integrity=\"sha384-wfSDF2E50Y2D1uUdj0O3uMBJnjuUD4Ih7YwaYd1iqfktj0Uod8GCExl3Og8ifwB6\" crossorigin=\"anonymous\"></script>"
        "</body>"
        "</html>";
    return htmlStatus;
}

void setup()
{
  // Attach Servo, start LittleFS and Connect to WiFi
  Serial.begin(9600);
  servo.attach(servoPin);
  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("\nConnected to the WiFi network: ");
  Serial.print(WiFi.SSID());
  Serial.print(" IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.printf("[HTTP GET /] Left: %s Right: %s\n", (currentStatus == dropLeft)?"Open":"Closed", (currentStatus == dropRight)?"Open":"Closed");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(buildHTMLString().c_str());
    request->send(response);
  });

  server.on("/dropleft", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentStatus = dropLeft;
    servo.write(currentStatus);
    Serial.printf("[HTTP GET /dropleft] currentStatus: %d\n", currentStatus);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(buildHTMLString().c_str());
    request->send(response);
  });

  server.on("/center", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentStatus = center;
    servo.write(currentStatus);
    Serial.printf("[HTTP GET /center] currentStatus: %d\n", currentStatus);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(buildHTMLString().c_str());
    request->send(response);
  });

  server.on("/dropright", HTTP_GET, [](AsyncWebServerRequest *request) {
    currentStatus = dropRight;
    servo.write(currentStatus);
    Serial.printf("[HTTP GET /dropright] currentStatus: %d\n", currentStatus);
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print(buildHTMLString().c_str());
    request->send(response);
  });

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.onNotFound([](AsyncWebServerRequest *request){
    Serial.printf("NOT_FOUND: ");
    if(request->method() == HTTP_GET)
      Serial.printf("GET");
    else if(request->method() == HTTP_POST)
      Serial.printf("POST");
    else if(request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if(request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if(request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if(request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if(request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if(request->contentLength()){
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }

    int headers = request->headers();
    int i;
    for(i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }

    int params = request->params();
    for(i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      } else {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }

    request->send(404);
  });

  server.begin();
}
void loop(){
}