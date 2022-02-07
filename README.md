# ёRadio
![ёRadio Logo](yoRadio/data/www/elogo100.png)

#### Web-radio based on [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S) library
#### More images in [Images.md](Images.md)
### Hardware
#### Required:
**ESP32 board**: https://aliexpress.ru/item/32847027609.html \
**I2S DAC**, roughly like this one: https://aliexpress.ru/item/1005001993192815.html
#### Optional:
##### Displays
- **ST7735** 1.8' or 1.44' https://aliexpress.ru/item/1005002822797745.html
- or **SSD1306** 0.96' I2C https://aliexpress.ru/item/1005001621806398.html
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
| PIN       | * | \
\* Any free pin, configured in options.h
## Version history
### v0.4.180
- Vol steps 0..256 (in ESP32-audioI2S)
### v0.4.177
- Added support for SSD1306 I2C displays.
- Fixed broken buttons.
