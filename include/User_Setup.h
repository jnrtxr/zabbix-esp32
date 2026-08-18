// Configuracao TFT_eSPI para ESP32-2432S028R (Cheap Yellow Display / CYD)
// Display: ILI9341 2.8" 320x240 | Chip USB: CH340
// Referencia: https://gist.github.com/panapol-p/9b2441b42fef15212b86e49722d39c63

#define ILI9341_2_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pinagem ESP32-2432S028R
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  12
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

#define TOUCH_CS 33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY       15999999
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  600000
