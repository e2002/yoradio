# ёRadio
![ёRadio Logo](yoRadio/data/www/elogo100.png)

#### Web-radio based on [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) library
![ёRadio](images/img0.jpg)
#### More images in [Images.md](Images.md)
### Hardware
#### Required:
**ESP32 board**: https://aliexpress.ru/item/32847027609.html \
**I2S DAC**, roughly like this one: https://aliexpress.ru/item/1005001993192815.html
#### Optional:
##### Displays
- **ST7735** 1.8' or 1.44' https://aliexpress.ru/item/1005002822797745.html
- or **SSD1306** 0.96' I2C https://aliexpress.ru/item/1005001621806398.html
- or **Nokia5110** 84x48 SPI https://aliexpress.ru/item/1005001621837569.html
##### Controls
Three tact buttons or Encoder or all together
### Connection table
| SPI Display | ESP-32 |
| ------ | ------ |
| GND | GND |
| VCC | +5v |
| SCL | 18 |
| SDA | 23 |
| CSL | 5* |
| RSTL | 15* |
| DCL | 4* |

| I2C Display | ESP-32 |
| ------ | ------ |
| GND | GND |
| VCC | +5v |
| SDA | 13* |
| SCL | 14* |

| I2S DAC | ESP-32 |
| ------ | ------ |
| GND       | GND |
| VIN       | +5v |
| DOUT(DIN) | 27* |
| BCLK      | 26* |
| LRC(WSEL) | 25* |

| Buttons, Encoder | ESP-32 |
| ------ | ------ |
| GND       | GND |
| PIN       | * |

\* Any free pin, configured in options.h
### Dependencies
#### Libraries:
Adafruit_GFX, Adafruit_ST7735\*, Adafruit_SSD1306\*, Adafruit_PCD8544\*, (\* depending on display model), ESP32Encoder, OneButton, [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer), [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
#### Tool:
[ESP32 Filesystem Uploader](https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/)
## Version history
### v0.4.197
- added support for Nokia 5110 SPI displays
- some bugs fixes
### v0.4.183
- ovol reading bug
### v0.4.182
- display connection algorithm changed
- added support for myoptions.h file for custom settings
### v0.4.180
- vol steps 0..256 (in ESP32-audioI2S)
### v0.4.177
- added support for SSD1306 I2C displays
- fixed broken buttons.
