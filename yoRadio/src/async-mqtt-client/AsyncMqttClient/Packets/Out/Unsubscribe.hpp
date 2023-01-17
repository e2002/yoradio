#pragma once

#include <cstring>  // strlen
#include <vector>

#include "OutPacket.hpp"
#include "../../Flags.hpp"
#include "../../Helpers.hpp"
#include "../../Storage.hpp"

namespace AsyncMqttClientInternals {
class UnsubscribeOutPacket : public OutPacket {
 public:
  explicit UnsubscribeOutPacket(const char* topic);
  const uint8_t* data(size_t index = 0) const;
  size_t size() const;

 private:
  std::vector<uint8_t> _data;
};
}  // namespace AsyncMqttClientInternals
