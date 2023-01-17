#include "Subscribe.hpp"

using AsyncMqttClientInternals::SubscribeOutPacket;

SubscribeOutPacket::SubscribeOutPacket(const char* topic, uint8_t qos) {
  char fixedHeader[5];
  fixedHeader[0] = AsyncMqttClientInternals::PacketType.SUBSCRIBE;
  fixedHeader[0] = fixedHeader[0] << 4;
  fixedHeader[0] = fixedHeader[0] | AsyncMqttClientInternals::HeaderFlag.SUBSCRIBE_RESERVED;

  uint16_t topicLength = strlen(topic);
  char topicLengthBytes[2];
  topicLengthBytes[0] = topicLength >> 8;
  topicLengthBytes[1] = topicLength & 0xFF;

  char qosByte[1];
  qosByte[0] = qos;

  uint8_t remainingLengthLength = AsyncMqttClientInternals::Helpers::encodeRemainingLength(2 + 2 + topicLength + 1, fixedHeader + 1);

  size_t neededSpace = 0;
  neededSpace += 1 + remainingLengthLength;
  neededSpace += 2;
  neededSpace += 2;
  neededSpace += topicLength;
  neededSpace += 1;

  _data.reserve(neededSpace);

  _packetId = _getNextPacketId();
  char packetIdBytes[2];
  packetIdBytes[0] = _packetId >> 8;
  packetIdBytes[1] = _packetId & 0xFF;

  _data.insert(_data.end(), fixedHeader, fixedHeader + 1 + remainingLengthLength);
  _data.insert(_data.end(), packetIdBytes, packetIdBytes + 2);
  _data.insert(_data.end(), topicLengthBytes, topicLengthBytes + 2);
  _data.insert(_data.end(), topic, topic + topicLength);
  _data.push_back(qosByte[0]);
  _released = false;
}

const uint8_t* SubscribeOutPacket::data(size_t index) const {
  return &_data.data()[index];
}

size_t SubscribeOutPacket::size() const {
  return _data.size();
}
