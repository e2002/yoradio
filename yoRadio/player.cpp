#include "options.h"

#include "player.h"

#include "config.h"
#include "telnet.h"
#include "display.h"

#include "netserver.h"

Player player;

#if VS1053_CS!=255 && !I2S_INTERNAL
Player::Player(): Audio(VS1053_CS, VS1053_DCS, VS1053_DREQ) {

}
void ResetChip(){
  pinMode(VS1053_RST, OUTPUT);
  digitalWrite(VS1053_RST, LOW);
  delay(30);
  digitalWrite(VS1053_RST, HIGH);
  delay(100);
}
#else
#if !I2S_INTERNAL
Player::Player() {}
#else
Player::Player(): Audio(true, I2S_DAC_CHANNEL_BOTH_EN)  {}
#endif
#endif


void Player::init() {
  if(MUTE_PIN!=255) pinMode(MUTE_PIN, OUTPUT);
#if I2S_DOUT!=255
#if !I2S_INTERNAL
  setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
#endif
#else
  SPI.begin();
  if(VS1053_RST>0) ResetChip();
  begin();
#endif
  setBalance(config.store.balance);
  setTone(config.store.bass, config.store.middle, config.store.trebble);
  setVolume(0);
  mode = STOPPED;
  setOutputPins(false);
  requestToStart = true;
  zeroRequest();
}

void Player::stopInfo() {
  config.setSmartStart(0);
  telnet.info();
  netserver.requestOnChange(MODE, 0);
  requestToStart = true;
}

void Player::loop() {
  if (mode == PLAYING) {
    Audio::loop();
  } else {
    if (isRunning()) {
      //digitalWrite(LED_BUILTIN, LOW);
      setOutputPins(false);
      config.setTitle((display.mode==LOST || display.mode==UPDATING)?"":"[stopped]");
      netserver.requestOnChange(TITLE, 0);
      //stopSong();
      setDefaults();
      stopInfo();
      if (player_on_stop_play) player_on_stop_play();
    }
  }
  if (request.station > 0) {
    if (request.doSave) {
      config.setLastStation(request.station);
    }
    play(request.station);
    if (player_on_station_change) player_on_station_change();
    zeroRequest();
  }
  if (request.volume >= 0) {
    config.setVolume(request.volume, request.doSave);
    telnet.printf("##CLI.VOL#: %d\n", config.store.volume);
    Audio::setVolume(volToI2S(request.volume));
    zeroRequest();
    display.putRequest({DRAWVOL, 0});
    netserver.requestOnChange(VOLUME, 0);
  }
}

void Player::zeroRequest() {
  request.station = 0;
  request.volume = -1;
  request.doSave = false;
}

void Player::setOutputPins(bool isPlaying) {
  digitalWrite(LED_BUILTIN, isPlaying);
  if(MUTE_PIN!=255) digitalWrite(MUTE_PIN, isPlaying?!MUTE_VAL:MUTE_VAL);
}

void Player::play(uint16_t stationId) {
  //stopSong();
  setDefaults();
  setOutputPins(false);
  config.setTitle("[connecting]");
  config.station.bitrate=0;
  netserver.requestOnChange(TITLE, 0);
  //telnet.printf("##CLI.META#: %s\n", config.station.title);
  config.loadStation(stationId);
  setVol(config.store.volume, true);
  display.putRequest({NEWSTATION, 0});
  netserver.requestOnChange(STATION, 0);
  telnet.printf("##CLI.NAMESET#: %d %s\n", config.store.lastStation, config.station.name);
  if (connecttohost(config.station.url)) {
    mode = PLAYING;
    config.setSmartStart(1);
    netserver.requestOnChange(MODE, 0);
    setOutputPins(true);
    requestToStart = true;
    if (player_on_start_play) player_on_start_play();
  }else{
    Serial.println("some unknown bug...");
  };
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
    //display.title("[stopped]");
  } else {
    request.station = config.store.lastStation;
  }
}

void Player::stepVol(bool up) {
  if (up) {
    if (config.store.volume <= 254 - VOL_STEP) {
      setVol(config.store.volume + VOL_STEP, false);
    }else{
      setVol(254, false);
    }
  } else {
    if (config.store.volume >= VOL_STEP) {
      setVol(config.store.volume - VOL_STEP, false);
    }else{
      setVol(0, false);
    }
  }
}

byte Player::volToI2S(byte volume) {
  int vol = map(volume, 0, 254 - config.station.ovol * 3 , 0, 254);
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
