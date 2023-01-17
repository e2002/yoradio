#pragma once

enum class AsyncMqttClientError : uint8_t {
  MAX_RETRIES = 0,
  OUT_OF_MEMORY = 1
};
