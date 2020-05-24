#include <OV2640.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiClient.h>

OV2640 cam;
WebServer server(80);

const char *ssid = "FRITZBox";
const char *password = "89020965368268527211";
unsigned long lastToggled;

void handle_jpg(void)
{
  WiFiClient client = server.client();

  cam.run();
  if (!client.connected())
  {
    Serial.println("fail...");
    return;
  }
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-disposition: inline; filename=capture.jpg\r\n";
  response += "Content-type: image/jpeg\r\n\r\n";
  server.sendContent(response);
  client.write((char *)cam.getfb(), cam.getSize());
  Serial.println("send image");
}

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

void setup()
{
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }

  camera_config_t camera_config;
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = 17;
  camera_config.pin_d1 = 35;
  camera_config.pin_d2 = 34;
  camera_config.pin_d3 = 5;
  camera_config.pin_d4 = 39;
  camera_config.pin_d5 = 18;
  camera_config.pin_d6 = 36;
  camera_config.pin_d7 = 19;
  camera_config.pin_xclk = 27;
  camera_config.pin_pclk = 21;
  camera_config.pin_vsync = 22;
  camera_config.pin_href = 26;
  camera_config.pin_sscb_sda = 25;
  camera_config.pin_sscb_scl = 23;
  camera_config.pin_reset = 15;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = CAMERA_PF_JPEG;
  camera_config.frame_size = CAMERA_FS_SVGA;

  cam.init(camera_config);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  lastToggled = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    if (millis() > lastToggled + 5000)
    {
      Serial.println("cannot connect, rebooting");
      ESP.restart();
    }
    Serial.print(F("."));
  }
  Serial.println(F("WiFi connected"));
  Serial.println("");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, handle_jpg);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    ESP.restart(); // just restart, because..why not
  }
  server.handleClient();
}
