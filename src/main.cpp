#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <lvgl.h>

// --- IMPORT YOUR UI ---
#include "ui/ui.h" // This links to your new UI folder

// --- HARDWARE PIN DEFINITIONS ---
#define TFT_BL 2
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0, 8, 4, 43, 0, 8, 4, 12, 1, 16000000);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(480, 272, bus, 0, true);

#define XPT_CS 38
#define XPT_IRQ 18
SPIClass mySpi = SPIClass(SPI);
XPT2046_Touchscreen ts(XPT_CS, XPT_IRQ);
WiFiManager wm;

// --- LVGL BUFFERS ---
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;

/* 1. FLUSH CALLBACK (Hardware Driver) */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

/* 2. INPUT CALLBACK (Hardware Driver) */
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    // REMEMBER: Adjust these map values based on your calibration!
    data->point.x = map(p.x, 200, 3800, 0, 480);
    data->point.y = map(p.y, 200, 3700, 0, 272);
    data->state = LV_INDEV_STATE_PR;
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

void setup()
{
  Serial.begin(115200);

  // 1. Init Hardware
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  mySpi.begin(12, 13, 11, 38);
  ts.begin(mySpi);
  ts.setRotation(1);

  // 2. Init LVGL
  lv_init();
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * 480 * 30, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, 480 * 30);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 272;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  // 3. Init WiFi (Optional: Move this after UI init if you want to show a loading screen)
  if (!wm.autoConnect("GreenGuardian_Setup"))
  {
    Serial.println("WiFi Failed");
  }
  else
  {
    Serial.println("WiFi Connected");
  }

  // 4. LOAD YOUR CUSTOM UI
  ui_init(); // <--- This function comes from your imported UI files
}

void loop()
{
  lv_timer_handler();

  // Example: Updating a label from the main loop
  // lv_label_set_text(ui_LabelTemperature, "25.5 C");

  delay(5);
}