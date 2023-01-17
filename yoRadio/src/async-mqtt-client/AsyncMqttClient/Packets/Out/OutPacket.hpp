#pragma once

#include <stdint.h>  // uint*_t
#include <stddef.h>  // size_t
#include <algorithm>  // std::min

#include "../../Flags.hpp"

namespace AsyncMqttClientInternals {
class OutPacket {
 public:
  OutPacket();
  virtual ~OutPacket();
  virtual const uint8_t* data(size_t index = 0) const = 0;
  virtual size_t size() const = 0;
  bool released() const;
  uint8_t packetType() const;
  uint16_t packetId() const;
  uint8_t qos() const;
  void release();

 public:
  OutPacket* next;
  uint32_t timeout;
  uint8_t noTries;

 protected:
  static uint16_t _getNextPacketId();
  bool _released;
  uint16_t _packetId;

 private:
  static uint16_t _nextPacketId;
};
}  // namespace AsyncMqttClientInternals
