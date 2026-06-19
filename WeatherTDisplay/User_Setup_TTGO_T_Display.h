// Copy this file into your TFT_eSPI library folder as User_Setup.h
// OR in Arduino IDE: edit User_Setup_Select.h and uncomment Setup25_TTGO_T_Display.h
//
// LilyGO TTGO T-Display (ESP32 Classic, 1.14" ST7789)

#define USER_SETUP_ID 25

#define ST7789_DRIVER
#define TFT_SDA_READ

#define TFT_WIDTH 135
#define TFT_HEIGHT 240

#define CGRAM_OFFSET

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4
#define TFT_BACKLIGHT_ON HIGH

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF

#define SMOOTH_FONT

#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 6000000
