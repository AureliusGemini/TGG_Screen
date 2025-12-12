#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFiManager.h> // Include WiFiManager

// --- PIN DEFINITIONS FOR SUNTON ESP32-4827S043R ---
#define TFT_BL 2
// RGB Interface Pins (Sunton Standard)
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
    0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
    1 /* pclk_active_neg */, 16000000 /* prefer_speed */
);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    480 /* width */, 272 /* height */, bus, 0 /* rotation */, true /* auto_flush */
);

// --- TOUCH SCREEN PINS (XPT2046) ---
// Note: Some Sunton boards use IO 38 for CS, others vary.
// If touch doesn't work, try defining SPI manually.
#define XPT_CS 38
#define XPT_IRQ 18
#define XPT_MOSI 11
#define XPT_MISO 13
#define XPT_CLK 12

SPIClass mySpi = SPIClass(SPI);
XPT2046_Touchscreen ts(XPT_CS, XPT_IRQ);
WiFiManager wm; // Initialize WiFiManager

// UI Function Prototypes
void drawUI();
void drawDataBox(int x, int y, const char *label, const char *value, uint16_t color);

void setup()
{
  Serial.begin(115200);

  // 1. Init Display
  gfx->begin();
  gfx->fillScreen(BLACK);

  // 2. Init Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 3. Init Touch
  mySpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // 4. WiFiManager Setup
  gfx->setCursor(10, 50);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->println("Initializing WiFi...");

  // Reset settings - wipe stored credentials for testing (Comment this out after first run!)
  // wm.resetSettings();

  // Custom configuration for the portal
  wm.setDebugOutput(true); // See logs in Serial Monitor

  // Callback when entering AP mode (to show info on screen)
  wm.setAPCallback([](WiFiManager *myWiFiManager)
                   {
    gfx->fillScreen(BLUE);
    gfx->setCursor(10, 10);
    gfx->setTextSize(2);
    gfx->println("Connect to WiFi:");
    gfx->println("ID: GreenGuardian_Setup");

    // Automatically connect using saved credentials, or start AP if failed
    bool res = wm.autoConnect("GreenGuardian_Setup");

    if (!res)
    {
      gfx->println("Failed to connect");
      // ESP.restart();
    }
    else
    {
      // 5. Connection Success
      gfx->fillScreen(BLACK);
      gfx->setCursor(10, 10);
      gfx->println("WiFi Connected!");
      gfx->print("IP: ");
      gfx->println(WiFi.localIP());
      delay(2000);

      // Draw the Main Dashboard
      gfx->fillScreen(BLACK);
      drawUI();
    } });
}

void loop()
{
  // Handle Touch
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();

    // Simple debug to see touch coordinates in Serial Monitor
    Serial.printf("X: %d, Y: %d, Z: %d\n", p.x, p.y, p.z);

    // Rudimentary calibration (You will likely need to tweak these map values)
    // Sunton 4827s043r raw values typically range ~200 to ~3800
    int touchX = map(p.x, 300, 3800, 0, 480);
    int touchY = map(p.y, 200, 3700, 0, 272);

    // Simple button logic example
    if (touchX > 0 && touchX < 480 && touchY > 0 && touchY < 50)
    {
      // Touched the header
      Serial.println("Header Touched");
    }
  }

  // Add your MQTT/Backend logic here to fetch real data

  delay(100);
}

// Reuse the UI logic
void drawUI()
{
  gfx->fillRect(0, 0, 480, 50, 0x07E0); // Green
  gfx->setTextColor(BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(80, 15);
  gfx->print("GREEN GUARDIAN");

  drawDataBox(20, 70, "TEMP", "24 C", 0xF800);      // Red
  drawDataBox(180, 70, "HUMIDITY", "60 %", 0x001F); // Blue
  drawDataBox(340, 70, "SOIL", "DRY", 0xFEA0);      // Orange
}

void drawDataBox(int x, int y, const char *label, const char *value, uint16_t color)
{
  gfx->drawRect(x, y, 120, 100, WHITE);
  gfx->fillRect(x, y, 120, 30, color);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(x + 10, y + 5);
  gfx->print(label);

  gfx->setTextColor(WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(x + 10, y + 50);
  gfx->print(value);
}