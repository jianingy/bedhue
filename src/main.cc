#include <NeoPixelBus.h>
#include <WiFi.h>
#include <aREST.h>

#include "secret.h"

const int PixelCount = 50;
const int PixelPin = 16;
const int SR501Pin = 21;
const int duration = 15000;
const int noise = 500;

unsigned long detectedAt = 0;
unsigned long disappearedAt = 0;
unsigned long now = millis();
bool detected = false;
bool illuminated = false;
int mode = 0;f-`

WiFiServer server(80);
aREST rest = aREST();
NeoPixelBus<NeoRgbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

void rainbow(int timeout) {
  RgbColor colors[] = {
                       RgbColor(160, 0, 0),
                       RgbColor(0, 160, 0),
                       RgbColor(0, 0, 160),
                       RgbColor(160, 160, 0),
                       RgbColor(0, 160, 160),
                       RgbColor(160, 0, 160)
  };
  for (int j = 0; j < sizeof(colors) / sizeof(RgbColor); j++) {
    strip.ClearTo(colors[j]);
    strip.Show();
    delay(timeout);
  }
}

void knight(RgbColor color, int count, int timeout) {
  if (count > PixelCount)
    return;
  strip.ClearTo(color, 0, count - 1);
  strip.Show();
  delay(timeout);
  // Go Right
  for (int i = count; i < PixelCount; i++) {
    strip.RotateRight(1);
    strip.Show();
    delay(timeout);
  }
  // Go Left
  for (int i = count; i < PixelCount; i++) {
    strip.RotateLeft(1);
    strip.Show();
    delay(timeout);
  }
}


void breath(HsbColor color, int interval, int p) {
  for (float i = 0; i < p; i++ ) {
    color.B += 0.01;
    strip.ClearTo(color);
    strip.Show();
    delay(interval / p);
  }
}

void solid(RgbColor color) {
  strip.ClearTo(color);
  strip.Show();
}

void flash(RgbColor color, int timeout) {
  solid(color);
  delay(timeout);
  solid(RgbColor(0));
  delay(timeout);
}

void onMotionChanged() {
  if (digitalRead(SR501Pin)) {
    Serial.println("Motion detected.");
    detected = true;
    detectedAt = millis();
  } else {
    Serial.println("Motion stopped.");
    detected = false;
    disappearedAt = millis();
  }
}

int switchMode(String command) {
  solid(RgbColor(0));
  mode = command.toInt();
  Serial.print("switch mode to ");
  Serial.println(mode);
  return mode;
}

void setup() {
  // initalize ws2812 strip
  strip.Begin();
  strip.ClearTo(RgbColor(0));
  strip.Show();

  // initialize serial port
  Serial.begin(115200);
  while (!Serial)
    flash(RgbColor(0, 64, 0), 10);

  pinMode(SR501Pin, INPUT);
  attachInterrupt(digitalPinToInterrupt(SR501Pin), onMotionChanged, CHANGE);

  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_SECRET);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    flash(RgbColor(0, 0, 64), 1000);
  }
  Serial.print("WiFi connected. IP address is ");
  Serial.println(WiFi.localIP());

  flash(RgbColor(64, 0, 0), 100);
  flash(RgbColor(64, 0, 0), 100);

  rest.function("switch", switchMode);

  server.begin();
  Serial.println("System started.");
}

void loop() {
  now = millis();

  WiFiClient client = server.available();
  if (client) {
    while(!client.available()){
      delay(1);
    }
    rest.handle(client);
  } else {
    if (mode < 10) {
      if (illuminated && !detected && (now - disappearedAt > duration)) {
        Serial.println("Illumination off.");
        solid(RgbColor(0));
        illuminated = false;
      } else if (detected && !illuminated && (now - detectedAt) > noise) {
        Serial.println("Illumination on.");
        if (mode == 0) {
          breath(HsbColor(0.3, 0.85, 0.0), 2000, 65);
        } else if (mode == 1) {
          knight(RgbColor(0, 160, 0), 3, 150);
          solid(RgbColor(0, 160, 0));
        } else if (mode == 2) {
          rainbow(250);
        }
        illuminated = true;
      }
    } else if (mode < 20) {
      if (mode == 10) {
        breath(HsbColor(0.3, 0.85, 0.0), 2000, 65);
      } else if (mode == 11) {
        knight(RgbColor(0, 160, 0), 3, 150);
      } else if (mode == 12) {
        rainbow(250);
      }
    }
  }
  delay(100);
}
