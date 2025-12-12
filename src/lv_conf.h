#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

// 1. Enable Color Depth
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0 // RGB565

// 2. Memory Settings
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE_KILOBYTES 64U // Use 64KB for LVGL internal heap

// 3. Tick Settings
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

// 4. Widget Settings (Enable things you might use)
#define LV_USE_LABEL 1
#define LV_USE_BTN 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_SLIDER 1
#define LV_USE_CHECKBOX 1
#define LV_USE_SWITCH 1

// 5. Font Settings
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#endif /*LV_CONF_H*/