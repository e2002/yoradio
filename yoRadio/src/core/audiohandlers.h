#ifndef AUDIOHANDLERS_H
#define AUDIOHANDLERS_H

//=============================================//
//              Audio handlers                 //
//=============================================//

void audio_info(const char *info) {
  if(player.lockOutput) return;
  if(config.store.audioinfo) telnet.printf("##AUDIO.INFO#: %s\n", info);
  #ifdef USE_NEXTION
    nextion.audioinfo(info);
  #endif
  if (strstr(info, "format is aac")  != NULL) { config.setBitrateFormat(BF_AAC); display.putRequest(DBITRATE); }
  if (strstr(info, "format is flac") != NULL) { config.setBitrateFormat(BF_FLAC); display.putRequest(DBITRATE); }
  if (strstr(info, "format is mp3")  != NULL) { config.setBitrateFormat(BF_MP3); display.putRequest(DBITRATE); }
  if (strstr(info, "format is wav")  != NULL) { config.setBitrateFormat(BF_WAV); display.putRequest(DBITRATE); }
  if (strstr(info, "skip metadata") != NULL) config.setTitle(config.station.name);
  if (strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL) {
    player.setError(info);
    
  }
  char* ici; char b[20]={0};
  if ((ici = strstr(info, "BitRate: ")) != NULL) {
    strlcpy(b, ici + 9, 50);
    audio_bitrate(b);
  }
}

void audio_bitrate(const char *info)
{
  if(config.store.audioinfo) telnet.printf("%s %s\n", "##AUDIO.BITRATE#:", info);
  config.station.bitrate = atoi(info) / 1000;
  display.putRequest(DBITRATE);
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
  bool p = printable(info) && (strlen(info) > 0);(void)p;
  //config.setTitle(p?info:config.station.name);
  if(player.remoteStationName){
    config.setStation(p?info:config.station.name);
    display.putRequest(NEWSTATION);
    netserver.requestOnChange(STATION, 0);
  }
}

void audio_showstreamtitle(const char *info) {
  DBGH();
  if (strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL) player.setError(info);
  bool p = printable(info) && (strlen(info) > 0);
  #ifdef DEBUG_TITLES
    config.setTitle(DEBUG_TITLES);
  #else
    config.setTitle(p?info:config.station.name);
  #endif
}

void audio_error(const char *info) {
  //config.setTitle(info);
  player.setError(info);
  telnet.printf("##ERROR#:\t%s\n", info);
}

void audio_id3artist(const char *info){
  if(printable(info)) config.setStation(info);
  display.putRequest(NEWSTATION);
  netserver.requestOnChange(STATION, 0);
}

void audio_id3album(const char *info){
  if(player.lockOutput) return;
  if(printable(info)){
    if(strlen(config.station.title)==0){
      config.setTitle(info);
    }else{
      char out[BUFLEN]= {0};
      strlcat(out, config.station.title, BUFLEN);
      strlcat(out, " - ", BUFLEN);
      strlcat(out, info, BUFLEN);
      config.setTitle(out);
    }
  }
}

void audio_id3title(const char *info){
  audio_id3album(info);
}

void audio_beginSDread(){
  config.setTitle("");
}

void audio_id3data(const char *info){  //id3 metadata
    if(player.lockOutput) return;
    telnet.printf("##AUDIO.ID3#: %s\n", info);
}

void audio_eof_mp3(const char *info){  //end of file
    config.sdResumePos = 0;
    if(config.sdSnuffle){
      player.sendCommand({PR_PLAY, random(1, config.store.countStation)});
    }else{
      player.next();
    }
}

void audio_eof_stream(const char *info){
  player.sendCommand({PR_STOP, 0});
  if(!player.resumeAfterUrl) return;
  if (config.store.play_mode==PM_WEB){
    player.sendCommand({PR_PLAY, config.store.lastStation});
  }else{
    player.setResumeFilePos( config.sdResumePos==0?0:config.sdResumePos-player.sd_min);
    player.sendCommand({PR_PLAY, config.store.lastStation});
  }
}

void audio_progress(uint32_t startpos, uint32_t endpos){
  player.sd_min = startpos;
  player.sd_max = endpos;
  netserver.requestOnChange(SDLEN, 0);
}

#endif
