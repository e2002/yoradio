void audio_info(const char *info) {
  if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
  if (strstr(info, "failed!") != NULL) {
    display.title("[Request failed!]");
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

void audio_showstation(const char *info) {
  if (strlen(info) > 0) {
    display.title(info);
    if (player.requesToStart) {
      telnet.info();
      player.requesToStart = false;
    } else {
      telnet.printf("##CLI.ICY0#: %s\n", info);
    }
  }
}

void audio_showstreamtitle(const char *info) {
  if (strlen(info) > 0) {
    display.title(info);
    if (player.requesToStart) {
      telnet.info();
      player.requesToStart = false;
    } else {
      telnet.printf("##CLI.META#: %s\n", info);
    }
  }
}
