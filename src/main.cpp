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
  String html = "// <!DOCTYPE HTML><html><head><meta name=/\"viewport\" content=\"width=device-width, initial-scale=1\"></head><<body>";
  html += "<h2>ESP Image Web Server</h2>";
  html += "<img src=\"fotka\">";
  html += "</body></html>";
}


bool checkPhoto( fs::FS &fs ) {         //funkcja sorawdzająca czy prawidłowo zapisany został plik z obrazem z kamery
  File f_pic = fs.open("/photo.jpg");
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}


 void fotka()   //funkcja do wykonania zdjęcia 
 {
    camera_fb_t * fb = NULL;
     bool ok = 0; 
  do {
     fb = esp_camera_fb_get();  //uruchomienie kamery 
    if (!fb) {
        Serial.println("Camera capture failed");
     }
       Serial.printf("Picture file name: %s\n", "photo.jpg");
    File file = SPIFFS.open("/photo.jpg", FILE_WRITE);  //otwarcie pliku w pamięci SPIFF
    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); //zapis obrazu w pamięci
      Serial.print("The picture has been saved in ");
      Serial.print("photo.jpg");
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }
    file.close();
    esp_camera_fb_return(fb);
    ok = checkPhoto(SPIFFS);
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

  Serial.print("Kamera gotowa wejdź na adres: 'http://");
  Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

 server.on("/fotka", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/photo.jpg", "image/jpg");
  });
  server.begin();
}

void loop() {
  delay(10000);
  fotka();
  Serial.printf("fotka ok");
}