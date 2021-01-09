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
#define CAMERA_MODEL_AI_THINKER     //wybór modelu kamery
#include "camera_pins.h"
#define DIODA 33

const char* ssid = "Lukas";
const char* password = "nalpinalpi";

AsyncWebServer server(80);  //użycie serwera asynchronicznego http na porcie 80

int pictureNumber = 0;
String pictureList[100];
String picturePath; 

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
    html +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
    html +="<meta http-equiv=\"refresh\" content=\"2;URL='http://esp32.local/'\">";
    html +="<title>ESP Image Web Server</title>\n";
    html +="<style>html { font-family: Helvetica; margin: 0px auto; text-align: center;}\n";
    html +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n</style>\n</head>\n<body>\n";
    html += "<h2>ESP Image Web Server</h2>\n";
  for(int i=0; i<pictureNumber; i++) {
    String pictureEndpoint = pictureList[i];
    html += "<img src=\"";
    html += pictureEndpoint + "\">\n";

    Serial.println("invoked endpoint for picture: " + pictureEndpoint);
  }
  html += "</body></html>\n";
  return html;
}



bool checkPhoto(fs::FS &fs, String photo) {         //funkcja sorawdzająca czy prawidłowo zapisany został plik z obrazem z kamery
  File f_pic = fs.open(photo);
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}


 void fotka(String photo)   //funkcja do wykonania zdjęcia 
 {
    photo += ".jpg"; 
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
}

void loop() {

  // zapis zdjęcia z nową nazwą z numerem
  pictureNumber++;
  String pictureNumberStr = String(pictureNumber);
  String pictureStr = "/photo";
  pictureStr += pictureNumberStr;
  fotka(pictureStr);
  Serial.println("picture taken properly");

  // dodanie do nazwy zdjecia znumerem do listy zdjęc
  pictureList[pictureNumber-1] = pictureStr;

  for(int i=0; i<pictureNumber; i++) {
    String endpointString = pictureList[i];
    const char * pictureEndpoint = endpointString.c_str(); 
    picturePath = endpointString + ".jpg";

    Serial.println("created endpoint (server.on): " + endpointString);
    Serial.println("created picturePath: " + picturePath);
    server.on(pictureEndpoint, HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, picturePath, "image/jpg");
    });
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", index_html());
  });
  server.begin();
  diode_one_blink();
  delay(10000);
}