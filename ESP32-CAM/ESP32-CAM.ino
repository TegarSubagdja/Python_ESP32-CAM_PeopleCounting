#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

WebServer server(80);

static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(640, 480);  // Adjusted midRes resolution
static auto hiRes = esp32cam::Resolution::find(800, 600);

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    server.send(503, "text/plain", "Capture failed");
    return;
  }

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void setAndServeJpg(const esp32cam::Resolution& resolution) {
  if (!esp32cam::Camera.changeResolution(resolution)) {
    Serial.println("SET-RES FAIL");
    server.send(500, "text/plain", "Failed to set resolution");
  } else {
    serveJpg();
  }
}

void handleJpgLo() {
  setAndServeJpg(loRes);
}

void handleJpgHi() {
  setAndServeJpg(hiRes);
}

void handleJpgMid() {
  setAndServeJpg(midRes);
}

void setup() {
  Serial.begin(9600);
  Serial.println();

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);  // Default to high resolution
    cfg.setBufferCount(2);
    cfg.setJpeg(80);

    if (psramFound()) {
      config.frame_size = FRAMESIZE_UXGA;  // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
      config.jpeg_quality = 10;            //10-63 lower number means higher quality
      config.fb_count = 2;
    } else {
      config.frame_size = FRAMESIZE_SVGA;
      config.jpeg_quality = 12;
      config.fb_count = 1;
    }

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
  }

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /cam-lo.jpg");
  Serial.println("  /cam-hi.jpg");
  Serial.println("  /cam-mid.jpg");

  server.on("/cam-lo.jpg", handleJpgLo);
  server.on("/cam-hi.jpg", handleJpgHi);
  server.on("/cam-mid.jpg", handleJpgMid);

  server.begin();
}

void loop() {
  server.handleClient();
}
