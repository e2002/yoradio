#include "options.h"

#include "WiFi.h"
#include "time.h"
#include "display.h"

#include "player.h"
#include "netserver.h"
#include "network.h"

#if DSP_MODEL==0
#include "src/displays/displayDummy.h"
DisplayDummy dsp;
#elif DSP_MODEL==1
#include "src/displays/displayST7735.h"
DisplayST7735 dsp;
#elif DSP_MODEL==2
#include "src/displays/displaySSD1306.h"
DisplaySSD1306 dsp;
#elif DSP_MODEL==3
#include "src/displays/displayN5110.h"
DisplayN5110 dsp;
#endif

Display display;

void ticks() {
  display.clockRequest = true;
}

#define STARTTIME  5000

#if DSP_MODEL==3
#define SCROLLDELTA 8
#define SCROLLTIME 332
#else
#define SCROLLDELTA 3
#define SCROLLTIME 83
#endif

void  Scroll::init(const char *sep, byte tsize, byte top, uint16_t dlay, uint16_t fgcolor, uint16_t bgcolor) {
  textsize = tsize;
  texttop = top;
  fg = fgcolor;
  bg = bgcolor;
  delayStartScroll=dlay;
  memset(separator, 0, 4);
  strlcpy(separator, sep, 4);
  locked = false;
}

void Scroll::setText(const char *txt) {
  memset(text, 0, BUFLEN / 2);
  strlcpy(text, txt, BUFLEN / 2);
  getbounds(textwidth, textheight, sepwidth);
  if (!locked) {
    clearscrolls();
    reset();
  }
}

void Scroll::lock() { locked = true; }

void Scroll::unlock() { locked = false; }

void Scroll::reset() {
  locked = false;
  clear();
  setTextParams();
  dsp.set_Cursor(TFT_FRAMEWDT, texttop);
  dsp.printText(text);
  drawFrame();
}

void Scroll::setTextParams() {
  dsp.set_TextSize(textsize);
  dsp.set_TextColor(fg, bg);
}

void Scroll::clearscrolls() {
  x = TFT_FRAMEWDT;
  scrolldelay = millis();
  clear();
}

void Scroll::loop() {
  if (checkdelay(x == TFT_FRAMEWDT ? delayStartScroll : SCROLLTIME, scrolldelay)) {
    scroll();
    sticks();
  }
  yield();
}

boolean Scroll::checkdelay(int m, unsigned long &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void Scroll::drawFrame() {
  dsp.drawScrollFrame(texttop, textheight, bg);
}

void Scroll::sticks() {
  if (!doscroll || locked) return;
  setTextParams();
  dsp.set_Cursor(x, texttop);
  dsp.printText(text);
  dsp.printText(separator);
  dsp.printText(text);
  drawFrame();
}

void Scroll::scroll() {
  if (!doscroll) return;
  if (textwidth > display.screenwidth) {
    x -= SCROLLDELTA;
    if (-x >= textwidth + sepwidth - TFT_FRAMEWDT) x = TFT_FRAMEWDT;
  }
}

void Scroll::clear() {
  dsp.clearScroll(texttop, textheight, bg);
}

void Scroll::getbounds(uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  if (strlen(text) == 0) {
    dsp.getScrolBbounds("EMPTY", separator, textsize, tWidth, tHeight, sWidth);
  } else {
    dsp.getScrolBbounds(text, separator, textsize, tWidth, tHeight, sWidth);
  }
  doscroll = (tWidth > display.screenwidth);
}

void Display::init() {
  dsp.initD(screenwidth, screenheight);
  dsp.drawLogo();
  if(DSP_MODEL==3){
    meta.init(" * ", 1, TFT_FRAMEWDT, STARTTIME, TFT_LOGO, TFT_BG);
    title1.init(" * ", 1, TFT_FRAMEWDT + TFT_LINEHGHT+1, STARTTIME, TFT_FG, TFT_BG);
    //title2.init(" * ", 1, TFT_FRAMEWDT + 2 * TFT_LINEHGHT, STARTTIME, TFT_FG, TFT_BG);
  }else{
    meta.init(" * ", 2, TFT_FRAMEWDT, STARTTIME, TFT_LOGO, TFT_BG);
    title1.init(" * ", 1, TFT_FRAMEWDT + 2 * TFT_LINEHGHT, STARTTIME, TFT_FG, TFT_BG);
    title2.init(" * ", 1, TFT_FRAMEWDT + 3 * TFT_LINEHGHT, STARTTIME, TFT_FG, TFT_BG);
  }
  int yStart = (screenheight / 2 - PLMITEMHEIGHT / 2) + 3;
  //plCurrent.init(" * ", 2, 57, 0, TFT_BG, TFT_LOGO);
  plCurrent.init(" * ", DSP_MODEL==3?1:2, yStart, 0, TFT_BG, TFT_LOGO);
  plCurrent.lock();
}

void Display::apScreen() {
  meta.setText(dsp.utf8Rus("ёRADIO * ёRADIO * ёRADIO", false));
  dsp.apScreen();
}

void Display::start() {
  clear();
  if (network.status != CONNECTED) {
    apScreen();
    return;
  }
  mode = PLAYER;
  title("[READY]");
  ip();
  volume();
  station();
  rssi();
  time();
  timer.attach_ms(1000, ticks);
  // Экстреминатус секвестирован
}

void Display::clear() {
  dsp.clearDsp();
}

void Display::swichMode(displayMode_e newmode) {
  if (newmode == VOL) {
    volDelay = millis();
  }
  if (newmode == mode) return;
  clear();
  mode = newmode;
  if (newmode != STATIONS) {
    ip();
    volume();
  }
  if (newmode == PLAYER) {
    meta.reset();
    title1.reset();
    if(DSP_MODEL!=3) title2.reset();
    plCurrent.lock();
    time();
  } else {
    meta.lock();
    title1.lock();
    if(DSP_MODEL!=3) title2.lock();
  }
  if (newmode == VOL) {
    dsp.frameTitle("VOLUME");
  }
  if (newmode == NUMBERS) {
    dsp.frameTitle("STATION");
  }
  if (newmode == STATIONS) {
    currentPlItem = config.store.lastStation;
    plCurrent.reset();
    drawPlaylist();
  }
}

void Display::drawPlayer() {
  if (clockRequest) {
    getLocalTime(&network.timeinfo);
    time();
    clockRequest = false;
  }
  meta.loop();
  title1.loop();
  if(DSP_MODEL!=3) title2.loop();
}

void Display::drawVolume() {
  if (millis() - volDelay > 3000) {
    volDelay = millis();
    swichMode(PLAYER);
  }
}

void Display::drawPlaylist() {
  char buf[PLMITEMLENGHT];
  dsp.drawPlaylist(currentPlItem, buf);
  plCurrent.setText(dsp.utf8Rus(buf, true));
}

void Display::drawNextStationNum(uint16_t num) {
  dsp.drawNextStationNum(num);
}

void Display::loop() {
  switch (mode) {
    case PLAYER: {
        drawPlayer();
        break;
      }
    case VOL: {
        drawVolume();
        break;
      }
    case NUMBERS: {
        break;
      }
    case STATIONS: {
        plCurrent.loop();
        break;
      }
  }
  dsp.loop();
  yield();
}

void Display::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  dsp.centerText(text, y, fg, bg);
}

void Display::bootString(const char* text, byte y) {
  dsp.centerText(text, y, TFT_LOGO, TFT_BG);
  dsp.loop();
}

void Display::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  dsp.rightText(text, y, fg, bg);
}

void Display::station() {
  meta.setText(dsp.utf8Rus(config.station.name, true));
  dsp.loop();
  netserver.requestOnChange(STATION, 0);
}

void Display::title(const char *str) {
  const char *title = str;
  char ttl[BUFLEN / 2] = { 0 };
  char sng[BUFLEN / 2] = { 0 };
  memset(config.station.title, 0, BUFLEN);
  strlcpy(config.station.title, title, BUFLEN);
  if (strlen(config.station.title) > 0) {
    char* ici;
    if ((ici = strstr(config.station.title, " - ")) != NULL && DSP_MODEL!=3) {
      strlcpy(sng, ici + 3, BUFLEN / 2);
      strlcpy(ttl, config.station.title, strlen(config.station.title) - strlen(ici) + 1);
    } else {
      strlcpy(ttl, config.station.title, BUFLEN / 2);
      sng[0] = '\0';
    }
    title1.setText(dsp.utf8Rus(ttl, true));
    if(DSP_MODEL!=3) title2.setText(dsp.utf8Rus(sng, true));
    dsp.loop();
  }
  netserver.requestOnChange(TITLE, 0);
}

void Display::heap() {
  if(config.store.audioinfo) dsp.displayHeapForDebug();
}

void Display::rssi() {
  char buf[20];
  int rssi = WiFi.RSSI();
  sprintf(buf, "%ddBm", rssi);
  dsp.rssi(buf);
  netserver.setRSSI(rssi);
}

void Display::ip() {
  dsp.ip(WiFi.localIP().toString().c_str());
}

void Display::time() {
  char timeStringBuff[20] = { 0 };
  if (!dt) {
    heap();
    rssi();
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &network.timeinfo);
  } else {
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H %M", &network.timeinfo);
  }
  dsp.printClock(timeStringBuff);
  dt = !dt;
}

void Display::volume() {
  dsp.drawVolumeBar(mode == VOL);
  netserver.requestOnChange(VOLUME, 0);
}
