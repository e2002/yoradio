#pragma once

namespace AsyncMqttClientInternals {
class Helpers {
 public:
  static uint32_t decodeRemainingLength(char* bytes) {
    uint32_t multiplier = 1;
    uint32_t value = 0;
    uint8_t currentByte = 0;
    uint8_t encodedByte;
    do {
      encodedByte = bytes[currentByte++];
      value += (encodedByte & 127) * multiplier;
      multiplier *= 128;
    } while ((encodedByte & 128) != 0);

    return value;
  }

  static uint8_t encodeRemainingLength(uint32_t remainingLength, char* destination) {
    uint8_t currentByte = 0;
    uint8_t bytesNeeded = 0;

    do {
      uint8_t encodedByte = remainingLength % 128;
      remainingLength /= 128;
      if (remainingLength > 0) {
        encodedByte = encodedByte | 128;
      }

      destination[currentByte++] = encodedByte;
      bytesNeeded++;
    } while (remainingLength > 0);

    return bytesNeeded;
  }
};

#if defined(ARDUINO_ARCH_ESP32)
  #define SEMAPHORE_TAKE() xSemaphoreTake(_xSemaphore, portMAX_DELAY)
  #define SEMAPHORE_GIVE() xSemaphoreGive(_xSemaphore)
  #define GET_FREE_MEMORY() ESP.getMaxAllocHeap()
  #include <esp32-hal-log.h>
#elif defined(ARDUINO_ARCH_ESP8266)
  #define SEMAPHORE_TAKE(X) while (_xSemaphore) { /*ESP.wdtFeed();*/ } _xSemaphore = true
  #define SEMAPHORE_GIVE() _xSemaphore = false
  #define GET_FREE_MEMORY() ESP.getMaxFreeBlockSize()
  #if defined(DEBUG_ESP_PORT) && defined(DEBUG_ASYNC_MQTT_CLIENT)
    #define log_i(...) DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.print("\n")
    #define log_e(...) DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.print("\n")
    #define log_w(...) DEBUG_ESP_PORT.printf(__VA_ARGS__); DEBUG_ESP_PORT.print("\n")
  #else
    #define log_i(...)
    #define log_e(...)
    #define log_w(...)
  #endif
#else
  #pragma error "No valid architecture"
#endif

}  // namespace AsyncMqttClientInternals
