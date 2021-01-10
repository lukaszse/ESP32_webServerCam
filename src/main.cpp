#include <WiFiClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_camera.h>
#include <Arduino.h>
#include <esp_timer.h>
#include <FS.h>
#include "ESPAsyncWebServer.h"
#include <SD.h>
#include <SPIFFS.h>
#include <NTPClient.h>
#define CAMERA_MODEL_AI_THINKER     //wybór modelu kamery
#include "camera_pins.h"
#define DIODA 33

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedTime;
String dayStamp;
String timeStamp;

const char* ssid = "Lukas";
const char* password = "nalpinalpi";

AsyncWebServer server(80);  //użycie serwera asynchronicznego http na porcie 80

int pictureNumber = 0;
int refreshFrequency = 10;

int startHour;
int endHour;
bool isWorking = true;

//prosta strona www z miejscem na obraz z kamery
// const char index_html[] PROGMEM = R"rawliteral(   
// <!DOCTYPE HTML><html>
// <head>
//   <meta name="viewport" content="width=device-width, initial-scale=1">
// </head>
// <body>
//   <h2>ESP Image Web Server</h2>
//   <img src="fotka">
/* //   </body>  
// </html>)rawliteral"; */

String index_html() {
  String html = "<!DOCTYPE html> <html>\n";
  html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  html += "<meta http-equiv=\"refresh\" content=\"" + String(refreshFrequency) + ";URL='http://esp32.local/'\">";
  html += "<title>ESP Image Web Server</title>\n";
  html += "<style>html { font-family: Helvetica; margin: 0px auto; text-align: center;}\n";
  html += "body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n</style>\n</head>\n<body>\n";
  html += "<h2>ESP Image Web Server</h2>\n";
  html += "<h3>Current time: </3>" + formattedTime + "\n";
  html += "\n";  
  html += "<div><input type=\"range\" min=\"1\" max=\"20\" value=\"10\" class=\"slider\" id=\"getFrequency\">\n";
  html += "\n";
  html += "<p>Reefresh frequency (per second): <span id=\"showFrequency\"></span></p></div>\n";
  html += "\n";
  html += "<form action=\"/startHour\">\n";
  html += "From hour (full hour): <input type=\"number\" id=\"fieldStartHour\" name=\"startHour\">\n";
  html += "<input type=\"submit\" value=\"Submit\">\n";
  html += "</form><br>\n";
  html += "<form action=\"/endHour\">\n";
  html += "To (full hour): <input type=\"number\" id=\"fieldEndHour\" name=\"startHour\">\n";
  html += "<input type=\"submit\" value=\"Submit\">\n";
  html += "</form><br>";

  if(isWorking) {
      html += "<H3>System is working  between: " + String(startHour) + " and " + String(endHour) + " hour." + "</H3>";
    for(int i=0; i<pictureNumber; i++) {
      String pictureEndpoint = "/fotka?number=" + String(i+1);
      html += "<img src=\"" + pictureEndpoint + "\">\n";

      Serial.println("invoked endpoint for picture: " + pictureEndpoint);
    }
  } else {
      html += "<H2>Pictures are not updating. System is working  only between: " + String(startHour) + " and " + String(endHour) + " hour." + "</H2>";
      html += "<h2>Set start and end hour.</H2>";
  }

  if(SPIFFS.totalBytes()-SPIFFS.usedBytes() == 0) {
    html += "Memory is full";
  }
  html += "\n";
  html += "\n";
  html += "<script>\n";
  html += "var fromHour = document.getElementById(\"fieldStartHour\");\n";         // wyslanie wartosci do formularz
  html += "var toHour = document.getElementById(\"fieldEndHour\");\n";             // wyslanie wartosci do formularz
  html += "var slider = document.getElementById(\"getFrequency\");\n";
  html += "var output = document.getElementById(\"showFrequency\");\n";

  html += "const req = new XMLHttpRequest();\n";
  html += "const url='/getFrequency';\n";
  html += "req.open(\"GET\", url, true);\n";
  html += "req.send();\n";

  html += "const reqStart = new XMLHttpRequest();\n";                              // wyslanie wartosci do formularza
  html += "const urlStart='/getStartHour';\n";                                     // wyslanie wartosci do formularza
  html += "reqStart.open(\"GET\", urlStart, true);\n";                             // wyslanie wartosci do formularza
  html += "reqStart.send();\n";                                                    // wyslanie wartosci do formularza

  html += "const reqEnd = new XMLHttpRequest();\n";                                // wyslanie wartosci do formularza
  html += "const urlEnd='/getEndHour';\n";                                         // wyslanie wartosci do formularza
  html += "reqEnd.open(\"GET\", urlEnd, true);\n";                                 // wyslanie wartosci do formularza
  html += "reqEnd.send();\n";                                                      // wyslanie wartosci do formularza

  html += "req.onreadystatechange = (e) => {\n";
  html += "var frequency = req.responseText;\n";
  html += "slider.value = frequency;\n";
  html += "output.innerHTML = frequency;\n";
  html += "}\n";

  html += "reqStart.onreadystatechange = (e) => {\n";
  html += "var oldStartHour = reqStart.responseText;\n";                            // wyslanie wartosci do formularz
  html += "fromHour.value = oldStartHour;\n";                                       // wyslanie wartosci do formularz
  html += "}\n";                       
                    
  html += "reqEnd.onreadystatechange = (e) => {\n";                      
  html += "var oldEndHour = reqEnd.responseText;\n";                               // wyslanie wartosci do formularz
  html += "toHour.value = oldEndHour;\n";                                           // wyslanie wartosci do formularz
  html += "}\n";

  html += "slider.oninput = function() {\n";
  html += "output.innerHTML = this.value;\n";
  html += "sendSlider(this.id, this.value);\n";
  html += "}\n";
  html += "function sendSlider(slider, value) {\n";
  html += "var http = new XMLHttpRequest();\n";
  html += "var url = '/refresh?frequency=' + value;\n";
  html += "http.open(\"GET\", url, true);\n";
  html += "http.send();\n";
  html += "}\n";
  html += "</script>\n";
  html += "</body></html>\n";
  return html;
}


bool checkPhoto(fs::FS &fs, String photo) {         //funkcja sorawdzająca czy prawidłowo zapisany został plik z obrazem z kamery
  File f_pic = fs.open(photo);
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}


 void fotka(int number)   //funkcja do wykonania zdjęcia 
 {
    String photo = "/photo" + String(number) + ".jpg";
    camera_fb_t * fb = NULL;
    bool ok = 0;
  do {
     fb = esp_camera_fb_get();  //uruchomienie kamery 
    if (!fb) {
        Serial.println("Camera capture failed");
     }
       Serial.printf("Picture file name: %s\n", photo.c_str());
    File file = SPIFFS.open(photo, FILE_WRITE);  //otwarcie pliku w pamięci SPIFF
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); //zapis obrazu w pamięci
      Serial.print("The picture has been saved in ");
      Serial.print(photo);
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file.close();
    esp_camera_fb_return(fb);
    ok = checkPhoto(SPIFFS, photo);
  } while (!ok);
 }

void diode_one_blink() {
  Serial.println("Dioda zał");
  digitalWrite(DIODA,LOW);
  delay(2000);
  Serial.println("Dioda wył");
  digitalWrite(DIODA,HIGH);

}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0; //definicja portów, do których podłączona jes kamera
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

if (!SPIFFS.begin(true)) {    //zamontowanie systemu plików w pamięci
    Serial.println("File system error");
    ESP.restart();
  }
  Serial.println("Memory ok");
  esp_err_t err = esp_camera_init(&config);   //inicjacja kamery
  if (err != ESP_OK) {
    Serial.printf("Camera initialization error: 0x%x", err);
    ESP.restart();
  }
  Serial.printf("Camera ok");

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected with WIFI");

  Serial.print("Camera is ready. Go to adress: 'http://");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/fotka", HTTP_GET, [](AsyncWebServerRequest *request) {
    String picNumber = request->getParam(0)->value();
    String picturePath = "/photo" + picNumber + ".jpg";
    request->send(SPIFFS, picturePath, "image/jpg");
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html());
  });
  server.on("/refresh", HTTP_GET, [](AsyncWebServerRequest *request){
    String refreshStr = request->getParam(0)->value();
    refreshFrequency = refreshStr.toInt();
    request->send(200, "text/html", index_html());
    Serial.println("Refresh frequncy set to: " +  refreshStr);
  });

  server.on("/getFrequency", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", String(refreshFrequency));
  });

  server.on("/startHour", HTTP_GET, [](AsyncWebServerRequest *request){
    String startHourStr = request->getParam(0)->value();
    startHour = startHourStr.toInt();
    request->send(200, "text/html", index_html());
  });

  server.on("/endHour", HTTP_GET, [](AsyncWebServerRequest *request){
    String endHourStr = request->getParam(0)->value();
    endHour = endHourStr.toInt();
    request->send(200, "text/html", index_html());
  });

  server.on("/getStartHour", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", String(startHour));
  });

  server.on("/getEndHour", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", String(endHour));
  });
  
  server.begin();
  pinMode(DIODA,OUTPUT);
  digitalWrite(DIODA,HIGH);

  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);

  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }

  formattedTime = timeClient.getFormattedTime();
  Serial.println("Current time: " + formattedTime);
}

void loop() {
  formattedTime = timeClient.getFormattedTime();
  int currentHour = timeClient.getHours();

  if(SPIFFS.totalBytes()-SPIFFS.usedBytes() == 0) {
    Serial.println("SPIFFS memory is full");
  } else {

    if(startHour<=endHour) {
        if(startHour<=currentHour && currentHour<=endHour) isWorking = true;
        else isWorking = false;
    } else {
        if(startHour<=currentHour || currentHour<=endHour) isWorking = true;
        else isWorking = false;
    }

  if(isWorking) { 
    if(pictureNumber == 10) {
      pictureNumber = 0;
      }
      pictureNumber++;
      fotka(pictureNumber);
      diode_one_blink();
      Serial.println();
      Serial.println("Invoking delay for: " + String(refreshFrequency));
      delay(refreshFrequency*1000);
    }
  }
}