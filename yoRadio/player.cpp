#include "player.h"
#include "config.h"
#include "telnet.h"
#include "display.h"
#include "options.h"
#include "netserver.h"

Player player;

void Player::init() {
  setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  setVolume(0);
  mode = STOPPED;
  requesToStart = true;
  setBalance(config.store.balance);
  setTone(config.store.bass, config.store.middle, config.store.trebble);
  zeroRequest();
}

void Player::stopInfo() {
  config.setSmartStart(0);
  telnet.info();
  netserver.requestOnChange(MODE, 0);
  requesToStart = true;
}

void Player::loop() {
  if (mode == PLAYING) {
    Audio::loop();
  } else {
    if (isRunning()) {
      digitalWrite(LED_BUILTIN, LOW);
      stopSong();
      stopInfo();
    }
  }
  if (request.station > 0) {
    if (request.doSave) {
      config.setLastStation(request.station);
    }
    play(request.station);
    zeroRequest();
  }
  if (request.volume >= 0) {
    config.setVolume(request.volume, request.doSave);
    display.volume();
    telnet.printf("##CLI.VOL#: %d\n", config.store.volume);
    Audio::setVolume(volToI2S(request.volume));
    zeroRequest();
  }
  yield();
}

void Player::zeroRequest() {
  request.station = 0;
  request.volume = -1;
  request.doSave = false;
}

void Player::play(byte stationId) {
  stopSong();
  digitalWrite(LED_BUILTIN, LOW);
  display.title("[connecting]");
  telnet.printf("##CLI.META#: %s\n", config.station.title);
  config.loadStation(stationId);
  setVol(config.store.volume, true);
  display.station();
  telnet.printf("##CLI.NAMESET#: %d %s\n", config.store.lastStation, config.station.name);
  if (connecttohost(config.station.url)) {
    mode = PLAYING;
    config.setSmartStart(1);
    netserver.requestOnChange(MODE, 0);
    digitalWrite(LED_BUILTIN, HIGH);
    requesToStart = true;
  }else{
    Serial.println("Some Unknown Bug...");
  }
  zeroRequest();
}

void Player::prev() {
  if (config.store.lastStation == 1) config.store.lastStation = config.store.countStation; else config.store.lastStation--;
  request.station = config.store.lastStation;
  request.doSave = true;
}

void Player::next() {
  if (config.store.lastStation == config.store.countStation) config.store.lastStation = 1; else config.store.lastStation++;
  request.station = config.store.lastStation;
  request.doSave = true;
}

void Player::toggle() {
  if (mode == PLAYING) {
    mode = STOPPED;
    display.title("[stopped]");
  } else {
    request.station = config.store.lastStation;
  }
}

void Player::stepVol(bool up) {
  if (up) {
    if (config.store.volume < 254) {
      setVol(config.store.volume + 1, false);
    }
  } else {
    if (config.store.volume > 0) {
      setVol(config.store.volume - 1, false);
    }
  }
}

byte Player::volToI2S(byte volume) {
  int vol = map(volume, 0, 254 - config.station.ovol * 2 , 0, 254);
  if (vol > 254) vol = 254;
  if (vol < 0) vol = 0;
  return vol;
}

void Player::setVol(byte volume, bool inside) {
  if (inside) {
    setVolume(volToI2S(volume));
  } else {
    request.volume = volume;
    request.doSave = true;
  }
}
