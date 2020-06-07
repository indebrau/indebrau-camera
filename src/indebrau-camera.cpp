#include <WebServer.h>
#include <WiFiClient.h>
#include <WiFiManager.h> // for configuring in access point mode

#include <esp_camera.h>
#include <soc/soc.h>          // Disable brownout problems
#include <soc/rtc_cntl_reg.h> // Disable brownout problems

// two boards: TTGO T-Journal or ESP32-CAM AI-Thinker are supported
#define ttgoBoard true

#if ttgoBoard
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 27
#define SIOD_GPIO_NUM 25
#define SIOC_GPIO_NUM 23
#define Y9_GPIO_NUM 19
#define Y8_GPIO_NUM 36
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 39
#define Y5_GPIO_NUM 5
#define Y4_GPIO_NUM 34
#define Y3_GPIO_NUM 35
#define Y2_GPIO_NUM 17
#define VSYNC_GPIO_NUM 22
#define HREF_GPIO_NUM 26
#define PCLK_GPIO_NUM 21
#else
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#endif

WebServer server(80);

unsigned long lastToggled;
WiFiManager wifiManager;
const char AP_PASSPHRASE[16] = "indebrau";
char deviceName[30];

void handleNotFound()
{
  String message = "Server is running!\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  server.send(200, "text/plain", message);
}

void handle_config(void)
{
  String message = "Portal started!\nPlease connect to Access Point\n";
  message += deviceName;
  message += "\n";
  server.send(200, "text/plain", message);
  wifiManager.startConfigPortal(deviceName, AP_PASSPHRASE);
}

void handle_jpg(void)
{
  WiFiClient client = server.client();

  camera_fb_t *fb = NULL; // pointer
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }
  esp_camera_fb_return(fb);

  if (!client.connected())
  {
    Serial.println("fail...");
    return;
  }
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-disposition: inline; filename=capture.jpg\r\n";
  response += "Content-type: image/jpeg\r\n\r\n";
  server.sendContent(response);
  client.write(fb->buf, fb->len);
}

void setup()
{
  Serial.begin(115200);
  // https://forum.arduino.cc/index.php?topic=613549.0
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8)
  {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  snprintf(deviceName, 30, "IndebrauCam-%08X", chipId); // set device name

  // WiFi Manager
  wifiManager.setTimeout(120);

  if (!wifiManager.autoConnect(deviceName, AP_PASSPHRASE))
  {
    Serial.println("Connection not possible, restart!");
    ESP.restart();
  }

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
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

#if ttgoBoard
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
#else
  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
#endif

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }

  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handle_jpg);
  server.on("/config", HTTP_GET, handle_config);

  server.onNotFound(handleNotFound);
  server.begin();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    /* If the wifimanager runs into a timeout, restart device
      Prevents being stuck in Access Point mode when Wifi signal was
      temporarily lost.
    */
    if (!wifiManager.autoConnect(deviceName, AP_PASSPHRASE))
    {
      Serial.println("Connection not possible, timeout, restart!");
      ESP.restart();
    }
  }
  server.handleClient();
}
