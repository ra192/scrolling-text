#include <Arduino.h>

#include "SPIFFS.h"
#include "Preferences.h"

#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"

#include <Adafruit_GFX.h>
#include <FastLED_NeoMatrix.h>
#include <FastLED.h>

#define LED_PIN1 14
#define LED_PIN2 27
#define LED_PIN3 26

#define WIDTH 32
#define HEIGHT 16

#define NUMMATRIX (WIDTH * HEIGHT)

#define TEXT_PREF "text"
#define TEXT_COLOR_PREF "text_color"

#define TEXT_SIZE 2
#define SYMBOL_SIZE_IN_PIXELS (6 * TEXT_SIZE)

Preferences myPrefs;

AsyncWebServer server(80);

CRGB leds[3 * NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
    leds, WIDTH, HEIGHT, 3, 1,
    NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + NEO_TILE_TOP + NEO_TILE_RIGHT + NEO_TILE_PROGRESSIVE);

String text;
uint16_t text_color;

void setup_prefs()
{
  myPrefs.begin("myPrefs", false);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  text = myPrefs.getString(TEXT_PREF, "Hello matrix");
  text_color = myPrefs.getInt(TEXT_COLOR_PREF, matrix->Color(0, 255, 0));
}

void setup_wifi_ap()
{
  WiFi.mode(WIFI_AP);
  // Connect to Wi-Fi network with SSID and password
  Serial.println("Setting AP (Access Point)");
  // NULL sets an open Access Point
  WiFi.softAP("led-matrix", NULL);
}

String processor(const String &var)
{
  if (var == "TEXT_TEMPLATE")
    return text;
  if(var == "TEXT_COLOR_TEMPLATE")
  {
    return String(text_color);
  }  
  return String();
}

void setup_webserver()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/matrix.html", "text/html", false, processor); });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request)
            {
      String text_param = request->getParam("text", true)->value();
      text = text_param;
      myPrefs.putString(TEXT_PREF, text);

      String color_param = request->getParam("color", true)->value();
      text_color=color_param.toInt();
      myPrefs.putInt(TEXT_COLOR_PREF, text_color);

      request->send(200, "text/plain", "Text Saved"); });

  server.serveStatic("/", SPIFFS, "/");

  server.begin();
}

void setup_led_matrix()
{
  FastLED.addLeds<NEOPIXEL, LED_PIN1>(leds, NUMMATRIX);
  FastLED.addLeds<NEOPIXEL, LED_PIN2>(leds, NUMMATRIX, NUMMATRIX);
  FastLED.addLeds<NEOPIXEL, LED_PIN3>(leds, 2 * NUMMATRIX, NUMMATRIX);

  matrix->begin();
  matrix->setTextSize(TEXT_SIZE);
  matrix->setTextWrap(false);
}

void setup()
{
  Serial.begin(115200);

  setup_prefs();
  setup_wifi_ap();
  setup_webserver();
  setup_led_matrix();
}

int x = 0;

void loop()
{
  matrix->fillScreen(0);
  matrix->setCursor(x, 1);
  matrix->setTextColor(text_color);
  matrix->print(text);
  matrix->show();
  
  int text_size_in_pixels = text.length() * SYMBOL_SIZE_IN_PIXELS;

  if (--x < -text_size_in_pixels)
    x = text_size_in_pixels;
  vTaskDelay(20 / portTICK_PERIOD_MS);
}