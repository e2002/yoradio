void audio_info(const char *info) {
  if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
  if (strstr(info, "failed!") != NULL || strstr(info, " 404") != NULL) {
    config.setTitle("[request failed]");
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
  telnet.printf("%s %s\n", "##AUDIO.BITRATE#:", info);
  config.station.bitrate = atoi(info) / 1000;
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
    config.setTitle(p?info:"*****");
    netserver.requestOnChange(TITLE, 0);
  }
}

void audio_showstreamtitle(const char *info) {
  if (strlen(info) > 0) {
    bool p = printable(info);
    config.setTitle(p?info:"*****");
    netserver.requestOnChange(TITLE, 0);
  }
}
