// TFT_eSPI setup for Waveshare ESP32-Touch-LCD-3.5 (ST7796, 320x480 portrait)
// Landscape UI uses setRotation(1) -> 480x320

#define USER_SETUP_ID 352

#define ST7796_DRIVER

#define TFT_WIDTH 320
#define TFT_HEIGHT 480

#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_MISO 19
#define TFT_CS 5
#define TFT_DC 27
// Reset is handled through TCA9554 (see board_init.h), not a direct GPIO
#define TFT_RST -1

#define TFT_BL 25
#define TFT_BACKLIGHT_ON HIGH

#define TFT_INVERSION_ON
#define TFT_RGB_ORDER TFT_BGR

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY 24000000
#define SPI_READ_FREQUENCY 6000000
