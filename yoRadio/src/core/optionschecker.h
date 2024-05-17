#ifndef optionschecker_h
#define optionschecker_h

#if YO_LED_BUILTIN==TFT_RST
#  error YO_LED_BUILTIN IS THE SAME AS TFT_RST. Check it in myoptions.h
#endif

#if YO_LED_BUILTIN==VS1053_RST
#  error YO_LED_BUILTIN IS THE SAME AS VS1053_RST. Check it in myoptions.h
#endif

#if (I2S_DOUT!=255) && (VS1053_CS!=255)
#  error YOU MUST CHOOSE BETWEEN I2S DAC AND VS1053 BY DISABLING THE SECOND MODULE IN THE myoptions.h
#endif

#if !defined(ARDUINO_ESP32_DEV) && !defined(ARDUINO_ESP32S3_DEV)
#  error ONLY MODULES "ESP32 Dev Module", "ESP32 Wrover Module" AND "ESP32-S3" ARE SUPPORTED. PLEASE SELECT ONE OF THEM IN THE MENU >> TOOLS >> BOARD
#endif

#endif


