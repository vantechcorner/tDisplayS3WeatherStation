# ESP32 Internet Weather Station

Black & white weather station UI for ESP32 boards with a small LCD. Based on the original [Volos Projects](https://www.youtube.com/watch?v=VntDY9Mg7T0) T-Display S3 build, extended for additional hardware.

| Board | Folder | Resolution | MCU |
|-------|--------|------------|-----|
| **TTGO T-Display 1.14"** (Classic) | `WeatherTDisplay/` | 240×135 | ESP32 |
| **Waveshare ESP32-Touch-LCD-3.5"** | `WeatherWaveshare35/` | 480×320 | ESP32 |
| **T-Display S3 1.9"** | `WeatherTDisplayS3/` | 320×170 | ESP32-S3 |

## Hardware

- [LilyGO TTGO T-Display 1.14"](https://lilygo.cc/products/lilygo%C2%AE-ttgo-t-display-1-14-inch-lcd-esp32-control-board) — ESP32 Classic
- [LilyGO T-Display S3 1.9"](https://www.lilygo.cc/0cAg0r) — ESP32-S3
- [Waveshare ESP32-Touch-LCD-3.5"](https://docs.waveshare.com/ESP32-Touch-LCD-3.5) — ESP32 Classic, ST7796 480×320

## Before you flash

**No API keys are included in this repository.** You need a free [OpenWeatherMap](https://openweathermap.org/api) API key:

1. Register at [openweathermap.org/api](https://openweathermap.org/api)
2. Copy your API key from the dashboard
3. New keys can take up to **2 hours** to activate

Enter the key using one of the methods below. **Do not commit your key to git or publish it in source code.**

---

## Quick test — pre-built firmware (Web Flash)

Pre-built binaries are provided for **TTGO T-Display 1.14"** and **Waveshare ESP32-Touch-LCD-3.5"** so you can try the firmware without installing PlatformIO or Arduino IDE.

### Download

| Board | Firmware folder | Flash size |
|-------|-----------------|------------|
| TTGO T-Display 1.14" | [`firmware/ttgo-t-display-1.14-inch/`](firmware/ttgo-t-display-1.14-inch/) | **4 MB** |
| Waveshare ESP32-Touch-LCD-3.5 | [`firmware/waveshare-35/`](firmware/waveshare-35/) | **16 MB** |

Each folder contains four files (same layout as a PlatformIO build):

| File | Flash offset |
|------|----------------|
| `bootloader.bin` | `0x1000` |
| `partitions.bin` | `0x8000` |
| `boot_app0.bin` | `0xE000` |
| `firmware.bin` | `0x10000` |

### Web flasher (Chrome or Edge)

Use a browser that supports **Web Serial** (Chrome or Edge). Recommended tools:

- [ESP Tool (Spacehuhn)](https://esptool.spacehuhn.com/) — manual multi-file flash
- [Espressif esptool-js](https://espressif.github.io/esptool-js/) — official web flasher

**Steps:**

1. Connect the board to your PC with a **USB data cable** (not charge-only).
2. Open one of the web flashers above.
3. Click **Connect** and select the board’s serial port (e.g. `COM12` on Windows).
4. Set **Flash size** to match your board: **4 MB** (TTGO) or **16 MB** (Waveshare).
5. Add all four `.bin` files from the correct `firmware/` folder using the offsets in the table above.
6. Click **Flash** / **Program** and wait until it completes (do not unplug during flash).
7. Press **Reset** or power-cycle the board.

**If the port does not appear:** hold the **BOOT** button, plug in USB, release BOOT, then click Connect again.

**If upload fails on TTGO:** some modules need BOOT held while starting the flash.

### After flashing

No API key is baked into the pre-built firmware. On first boot:

1. The board opens a WiFi setup portal — connect to **`VolosWifiConf`** (password **`password`**).
2. Open the config page in your browser.
3. Enter your home WiFi credentials and your [OpenWeatherMap](https://openweathermap.org/api) API key.
4. Save. Settings are stored in flash and kept after reboot.

To re-open the portal later, hold **BOOT** for about 3 seconds.

### Building the firmware bundles (maintainers)

Build from source, then copy the output files into `firmware/`:

```bash
# TTGO T-Display 1.14"
pio run -e ttgo-t-display
```

Copy from `.pio/build/ttgo-t-display/`: `bootloader.bin`, `partitions.bin`, `firmware.bin`  
Also copy `boot_app0.bin` from `.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin`  
Place all four files in `firmware/ttgo-t-display-1.14-inch/`.

```bash
# Waveshare ESP32-Touch-LCD-3.5
pio run -c platformio-waveshare.ini -e waveshare-35
```

Copy the same four files from `.pio/build/waveshare-35/` into `firmware/waveshare-35/`.

---

## TTGO T-Display 1.14" (ESP32 Classic)

Compact UI tailored for the 240×135 screen. Uses latitude/longitude for the weather query (more reliable than city names).

### PlatformIO (recommended)

```bash
pio run -e ttgo-t-display -t upload
```

TFT_eSPI pin config is applied automatically via `User_Setup_TTGO_T_Display.h`.

### Arduino IDE

1. Board: **ESP32 Dev Module** — PSRAM **Disabled**, Flash **4MB**
2. Libraries: WiFiManager 2.0.17, TFT_eSPI 2.5.x, ArduinoJson 7.1.0, ESP32Time 2.0.6
3. TFT_eSPI: use `User_Setups/Setup25_TTGO_T_Display.h`, or copy `WeatherTDisplay/User_Setup_TTGO_T_Display.h` to `TFT_eSPI/User_Setup.h`
4. Open `WeatherTDisplay/WeatherTDisplay.ino`
5. Edit timezone, coordinates, and units at the top of the sketch
6. Upload (hold **BOOT** if upload fails)

### Configuration

```cpp
int zone = 7;                    // UTC offset (e.g. 7 for Vietnam)
const float cityLat = 16.0544f;  // your city latitude
const float cityLon = 108.2022f; // your city longitude
String units = "metric";       // metric or imperial
String myAPI = "";             // leave empty — use setup portal
```

### API key via WiFi portal

On first boot (or hold **BOOT** for 3 seconds):

1. Connect to WiFi **`VolosWifiConf`** (password **`password`**)
2. Open the captive portal in your browser
3. Enter WiFi credentials and paste your key in **OpenWeather API Key**
4. Save — the key is stored in NVS and persists across reboots

Alternatively, set `myAPI = "your_key_here"` in the sketch and reflash.

---

## Waveshare ESP32-Touch-LCD-3.5

Full original author UI (custom fonts, 12-hour temperature graph) scaled from 320×170 to 480×320.

**Important:** LCD power and reset go through a **TCA9554** IO expander (I2C `0x20`) and power rails through **AXP2101**. Both are initialized in `board_init.h` before `tft.init()`. Without this, the backlight turns on but the screen stays black.

### PlatformIO

```bash
pio run -c platformio-waveshare.ini -e waveshare-35 -t upload
```

Board settings: ESP32 Dev Module, 16MB flash, PSRAM enabled. TFT config: `User_Setup_Waveshare_ESP32_Touch_LCD_35.h`.

### Arduino IDE

1. Board: **ESP32 Dev Module** — Flash **16MB**, PSRAM **Enabled**
2. Copy `WeatherWaveshare35/User_Setup_Waveshare_ESP32_Touch_LCD_35.h` to `TFT_eSPI/User_Setup.h`
3. Open `WeatherWaveshare35/WeatherWaveshare35.ino`
4. Edit timezone, city, and units at the top of the sketch
5. Enter API key via the WiFi setup portal (same as TTGO above) or set `myAPI` in code

---

## T-Display S3 1.9"

Original sketch from the Volos Projects YouTube build. Uses city name for the weather query.

### PlatformIO

```bash
pio run -e t-display-s3 -t upload
```

### Arduino IDE

1. Configure TFT_eSPI for T-Display S3 (see LilyGO examples)
2. Open `WeatherTDisplayS3/WeatherTDisplayS3.ino`
3. Set `town`, `zone`, `units`, and **`myAPI`** at the top of the sketch — this variant does not have a WiFi portal field for the API key

```cpp
int zone = 2;
String town = "Your City";
String myAPI = "";  // required — paste your OpenWeatherMap key here
String units = "metric";
```

---

## Troubleshooting

| Symptom | Likely cause |
|---------|----------------|
| HTTP **401** | Invalid or inactive API key — wait up to 2 h after registration |
| HTTP **400** | Bad city name — use lat/lon (TTGO/Waveshare) or check spelling (S3) |
| Black screen (Waveshare) | Missing TCA9554/AXP2101 init — ensure `board_init.h` runs before `tft.init()` |
| Upload fails | Hold **BOOT** while connecting, or check COM port / USB cable |

---

## Credits

- Original design and S3 firmware: [Volos Projects](https://www.youtube.com/watch?v=VntDY9Mg7T0)
- TTGO T-Display 1.14" and Waveshare 3.5" ports: **[Van Tech Corner](https://www.youtube.com/@VanTechCorner)** · [GitHub](https://github.com/vantechcorner)

<a href="http://www.youtube.com/watch?feature=player_embedded&v=VntDY9Mg7T0" target="_blank"><img src="http://img.youtube.com/vi/VntDY9Mg7T0/0.jpg" alt="Original YouTube build" width="480" height="320" border="10" /></a>
