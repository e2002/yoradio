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
  #ifndef CS
    #define CS 3
  #endif
  if(L10N_LANGUAGE!=RU && L10N_LANGUAGE!=CS) return true;
  // Validate the whole string as printable ASCII and/or well-formed UTF-8.
  // Unlike the old first-byte-only heuristic, this also accepts mixed strings
  // such as "Rádio Česko" (ASCII lead + diacritics) while still rejecting
  // binary garbage that some streams put in ICY metadata.
  size_t len = strlen(info);
  if (!len) return false;
  for (size_t c = 0; c < len; ) {
    uint8_t b = (uint8_t)info[c];
    if (b >= 0x20 && b <= 0x7e) { c++; continue; }        // printable ASCII
    int nb;                                              // expected continuation bytes
    if      (b >= 0xC2 && b <= 0xDF) nb = 1;             // 2-byte
    else if (b == 0xE0)              nb = 2;             // 3-byte (2nd >= 0xA0)
    else if (b >= 0xE1 && b <= 0xEC) nb = 2;
    else if (b == 0xED)              nb = 2;             // 3-byte (2nd <= 0x9F, no surrog)
    else if (b >= 0xEE && b <= 0xEF) nb = 2;
    else if (b == 0xF0)              nb = 3;             // 4-byte (2nd >= 0x90)
    else if (b >= 0xF1 && b <= 0xF3) nb = 3;
    else if (b == 0xF4)              nb = 3;             // 4-byte (2nd <= 0x8F)
    else return false;                                   // bare continuation / invalid lead / control
    if (c + nb >= len) return false;                     // truncated sequence
    for (int i = 1; i <= nb; i++) {
      uint8_t cb = (uint8_t)info[c + i];
      if (cb < 0x80 || cb > 0xBF) return false;
    }
    c += nb + 1;
  }
  return true;
}

void audio_showstation(const char *info) {
  bool p = printable(info) && (strlen(info) > 0);(void)p;
  if(player.remoteStationName){
    config.setStation(p?info:config.station.name);
    display.putRequest(NEWSTATION);
    netserver.requestOnChange(STATION, 0);
  }
}

void audio_showstreamtitle(const char *info) {
  if (strstr(info, "Account already in use") != NULL || strstr(info, "HTTP/1.0 401") != NULL || strstr(info, "HTTP/1.1 401") != NULL) player.setError(info);
  bool p = printable(info) && (strlen(info) > 0);
  #ifdef DEBUG_TITLES
    config.setTitle(DEBUG_TITLES);
  #else
    config.setTitle(p?info:config.station.name);
  #endif
}

void audio_error(const char *info) {
  player.setError(info);
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
      char tmp[BUFLEN+3];
      snprintf(tmp, BUFLEN+3, "%s - %s", config.station.title, info);
      config.setTitle(tmp);
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
    player.next();
}

void audio_eof_stream(const char *info){
  player.sendCommand({PR_STOP, 0});
  if(!player.resumeAfterUrl) return;
  if (config.getMode()==PM_WEB){
    player.sendCommand({PR_PLAY, config.lastStation()});
  }else{
    player.setResumeFilePos( config.sdResumePos==0?0:config.sdResumePos-player.sd_min);
    player.sendCommand({PR_PLAY, config.lastStation()});
  }
}

void audio_progress(uint32_t startpos, uint32_t endpos){
  player.sd_min = startpos;
  player.sd_max = endpos;
  netserver.requestOnChange(SDLEN, 0);
}

#endif
