#pragma once

#include "OutPacket.hpp"
#include "../../Flags.hpp"
#include "../../Helpers.hpp"

namespace AsyncMqttClientInternals {
class PingReqOutPacket : public OutPacket {
 public:
  PingReqOutPacket();
  const uint8_t* data(size_t index = 0) const;
  size_t size() const;

 private:
  uint8_t _data[2];
};
}  // namespace AsyncMqttClientInternals
