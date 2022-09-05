void audio_info(const char *info) {
  if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
#ifdef USE_NEXTION
  if (strstr(info, "format is aac")  != NULL) nextion.bitratePic(ICON_AAC);
  if (strstr(info, "format is flac") != NULL) nextion.bitratePic(ICON_FLAC);
  if (strstr(info, "format is mp3")  != NULL) nextion.bitratePic(ICON_MP3);
  if (strstr(info, "format is wav")  != NULL) nextion.bitratePic(ICON_WAV);
#endif
  if (strstr(info, "failed!") != NULL || strstr(info, " 404") != NULL || strstr(info, " 403") != NULL || strstr(info, "address is empty") != NULL) {
    config.setTitle("[request failed]");
    netserver.requestOnChange(TITLE, 0);
    player.setOutputPins(false);
    player.setDefaults();
    if (player_on_stop_play) player_on_stop_play();
    player.mode = STOPPED;
    player.stopInfo();
  }
  if (strstr(info, "not supported") != NULL || strstr(info, "Account already in use") != NULL){
    config.setTitle(info);
    netserver.requestOnChange(TITLE, 0);
    player.setOutputPins(false);
    player.setDefaults();
    if (player_on_stop_play) player_on_stop_play();
    player.mode = STOPPED;
    player.stopInfo();
  }
}

void audio_bitrate(const char *info)
{
  if(config.store.audioinfo) telnet.printf("%s %s\n", "##AUDIO.BITRATE#:", info);
  config.station.bitrate = atoi(info) / 1000;
#ifdef USE_NEXTION
  nextion.bitrate(config.station.bitrate);
#endif
  netserver.requestOnChange(BITRATE, 0);
}

bool printable(const char *info) {
  bool p = true;
  for (int c = 0; c < strlen(info); c++)
  {
    if ((uint8_t)info[c] > 0x7e || (uint8_t)info[c] < 0x20) p = false;
  }
  if (!p) p = (uint8_t)info[0] >= 0xC2 && (uint8_t)info[1] >= 0x80 && (uint8_t)info[1] <= 0xBF;
  return p;
}

void audio_showstation(const char *info) {
  if (strlen(info) > 0) {
    bool p = printable(info);
    config.setTitle(p?info:config.station.name);
    netserver.requestOnChange(TITLE, 0);
  }
}

void audio_showstreamtitle(const char *info) {
  DBGH();
  if (strstr(info, "Account already in use") != NULL){
    config.setTitle(info);
    netserver.requestOnChange(TITLE, 0);
    player.setOutputPins(false);
    player.setDefaults();
    if (player_on_stop_play) player_on_stop_play();
    player.mode = STOPPED;
    player.stopInfo();
    return;
  }
  if (strlen(info) > 0) {
    bool p = printable(info);
#ifdef DEBUG_TITLES
    config.setTitle(DEBUG_TITLES);
#else
    config.setTitle(p?info:config.station.name);
#endif
    netserver.requestOnChange(TITLE, 0);
  }
}
