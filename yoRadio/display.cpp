#include "options.h"

#include "WiFi.h"
#include "time.h"
#include "display.h"

#include "player.h"
#include "netserver.h"
#include "network.h"

DspCore dsp;
Display display;

#ifndef DUMMYDISPLAY
void ticks() {
  display.putRequest({CLOCK,0});
}

#ifndef STARTTIME
#define STARTTIME  5000
#endif
#ifndef STARTTIME_PL
#define STARTTIME_PL  0
#endif
#ifndef META_SIZE
#define META_SIZE   2
#endif
#ifndef TITLE_SIZE1
#define TITLE_SIZE1  1
#endif
#ifndef TITLE_SIZE2
#define TITLE_SIZE2  1
#endif
#ifndef TITLE_TOP1
#define TITLE_TOP1 TFT_FRAMEWDT + META_SIZE * TFT_LINEHGHT
#endif
#ifndef TITLE_TOP2
#define TITLE_TOP2 TFT_FRAMEWDT + (META_SIZE+2) * TFT_LINEHGHT
#endif
#ifndef TITLE_FG1
#define TITLE_FG1 TFT_FG
#endif
#ifndef TITLE_FG2
#define TITLE_FG2 TFT_FG
#endif
#ifndef BOOTSTR_TOP1
#define BOOTSTR_TOP1 90
#endif
#ifndef BOOTSTR_TOP2
#define BOOTSTR_TOP2 110
#endif
#ifndef PLCURRENT_SIZE
#define PLCURRENT_SIZE  2
#endif

#ifndef CLOCK_SPACE
#define DO_SCROLL (tWidth > display.screenwidth - TFT_FRAMEWDT * 2)
#else
#define DO_SCROLL (tWidth > (display.screenwidth - (dsp.fillSpaces?((texttop==0)?CLOCK_SPACE:VOL_SPACE):0)))
#endif

#ifndef CORE_STACK_SIZE
#define CORE_STACK_SIZE  1024*3
#endif

byte currentScrollId = 0;   /* one scroll on one time */

void  Scroll::init(byte ScrollId, const char *sep, byte tsize, byte top, uint16_t dlay, uint16_t fgcolor, uint16_t bgcolor) {
  textsize = tsize;
  id = ScrollId;
  if (textsize == 0) return;
  texttop = top;
  fg = fgcolor;
  bg = bgcolor;
  delayStartScroll = dlay;
  memset(separator, 0, 4);
  strlcpy(separator, sep, 4);
  locked = false;
}

void Scroll::setText(const char *txt) {
  if (textsize == 0) return;
  memset(text, 0, BUFLEN / 2);
  strlcpy(text, txt, BUFLEN / 2 - 1);
  getbounds(textwidth, textheight, sepwidth);
  if (!locked) {
    clearscrolls();
    reset();
  }
  lockRequest = false;
}

void Scroll::lock() {
  locked = true;
}

void Scroll::unlock() {
  locked = false;
}

void Scroll::reset() {
  if (textsize == 0) return;
  locked = false;
  clear();
  setTextParams();
  dsp.set_Cursor(TFT_FRAMEWDT, texttop);
  dsp.printText(text);
  drawFrame();
  if (currentScrollId == id) currentScrollId = 0;
}

void Scroll::setTextParams() {
  if (textsize == 0) return;
  dsp.set_TextSize(textsize);
  dsp.set_TextColor(fg, bg);
}

void Scroll::clearscrolls() {
  if (textsize == 0) return;
  x = TFT_FRAMEWDT;
  scrolldelay = millis();
  //clear();
}

void Scroll::loop() {
  if (lockRequest) {
    return;
  }
  if (textsize == 0) return;
  if (currentScrollId != 0 && currentScrollId != id) {
    return;
  }
  if (checkdelay(x == TFT_FRAMEWDT ? delayStartScroll : SCROLLTIME, scrolldelay)) {
    scroll();
    sticks();
  }
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
  if (textsize == 0) return;
  dsp.drawScrollFrame(texttop, textheight, bg);
}

void Scroll::sticks() {
  if (!doscroll || locked || textsize == 0) return;
  setTextParams();
  dsp.set_Cursor(x, texttop);
  dsp.printText(text);
  dsp.printText(separator);
  dsp.printText(text);
  if (x == TFT_FRAMEWDT) drawFrame();
}

void Scroll::scroll() {
  if (!doscroll || textsize == 0) return;

  //if (textwidth > display.screenwidth) {
  x -= SCROLLDELTA;
  if (-x > textwidth + sepwidth - TFT_FRAMEWDT) {
    x = TFT_FRAMEWDT;
    drawFrame();
    currentScrollId = 0;
  } else {
    currentScrollId = id;
  }
  //}
}

void Scroll::clear() {
  if (textsize == 0) return;
  dsp.clearScroll(texttop, textheight, bg);
}

void Scroll::getbounds(uint16_t &tWidth, uint16_t &tHeight, uint16_t &sWidth) {
  if (textsize == 0) return;
  if (strlen(text) == 0) {
    dsp.getScrolBbounds("EMPTY", separator, textsize, tWidth, tHeight, sWidth);
  } else {
    dsp.getScrolBbounds(text, separator, textsize, tWidth, tHeight, sWidth);
  }
  doscroll = DO_SCROLL;
}


TaskHandle_t TaskCore0;
QueueHandle_t displayQueue;

void Display::createCore0Task(){
  xTaskCreatePinnedToCore(
      loopCore0,                  /* Task function. */
      "TaskCore0",                /* name of task. */
      CORE_STACK_SIZE,            /* Stack size of task */
      NULL,                       /* parameter of the task */
      4,                          /* no one flies higher than the Toruk */
      &TaskCore0,                 /* Task handle to keep track of created task */
      !xPortGetCoreID());         /* pin task to core 0 */  
  //delay(500);
}

void loopCore0( void * pvParameters ){
  delay(500);
  while(!display.busy){
    if(displayQueue==NULL) break;
    display.loop();
    vTaskDelay(10);
  }
  vTaskDelete( NULL );
  TaskCore0=NULL;
}

void Display::init() {
  dsp.initD(screenwidth, screenheight);
  dsp.drawLogo();
  meta.init(1, " * ", META_SIZE, TFT_FRAMEWDT, STARTTIME, TFT_LOGO, TFT_BG);
  title1.init(2, " * ", TITLE_SIZE1, TITLE_TOP1, STARTTIME, TITLE_FG1, TFT_BG);
  title2.init(3, " * ", TITLE_SIZE2, TITLE_TOP2, STARTTIME, TITLE_FG2, TFT_BG);
  int yStart = (screenheight / 2 - PLMITEMHEIGHT / 2) + 3;
#ifdef PL_TOP
  yStart = PL_TOP;
#endif
  plCurrent.init(4, " * ", PLCURRENT_SIZE, yStart, STARTTIME_PL, TFT_BG, TFT_LOGO);
  plCurrent.lock();
  if (dsp_on_init) dsp_on_init();
}

void Display::apScreen() {
#ifndef PL_TOP
  meta.setText(dsp.utf8Rus("ёRADIO * ёRADIO * ёRADIO", false));
#endif
  dsp.apScreen();
}

void Display::stop() {
  busy = true;
  swichMode(PLAYER);
  timer.detach();
  resetQueue();
  vQueueDelete(displayQueue);
  displayQueue=NULL;
  delay(100);
  bootLogo();
  bootString("reloading", 1);
  busy = false;
}

void Display::start(bool reboot) {
  busy = false;
  displayQueue = xQueueCreate( 5, sizeof( requestParams_t ) );
  createCore0Task();

  clear();
  if (network.status != CONNECTED) {
    apScreen();
    return;
  }
  mode = PLAYER;
  if(!reboot){
    config.setTitle("[READY]");
    //loop();
  }
  ip();
  volume();
  station();
  rssi();
  time(reboot);
  if(reboot) title();
  timer.attach_ms(1000, ticks);
  if (dsp_on_start) dsp_on_start(&dsp);
  // Экстреминатус секвестирован /*дважды*/ /*трижды*/ /*четырежды*/ пятирежды
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
    currentScrollId = 0;
    meta.reset();
    title1.reset();
    if (TITLE_SIZE2 != 0) title2.reset();
    //player.loop();
    plCurrent.lock();
    time(true);
#ifdef CLOCK_SPACE  // if set space for clock in 1602 displays
    dsp.fillSpaces = true;
    ip();
    rssi();
    volume();
#endif
    dsp.loop(true);
  } else {
    if (newmode != NUMBERS) {
      meta.lock();
    }
    title1.lock();
    if (TITLE_SIZE2 != 0) title2.lock();
#ifdef CLOCK_SPACE
    dsp.fillSpaces = false;
#endif
  }
  if (newmode == VOL) {
#ifdef IP_INST_VOL
    dsp.frameTitle(WiFi.localIP().toString().c_str());
#else
    dsp.frameTitle("VOLUME");
#endif
  }
  if (newmode == LOST) {
    dsp.frameTitle("* LOST *");
  }
  if (newmode == NUMBERS) {
    //dsp.frameTitle("STATION");
    meta.reset();
  }
  if (newmode == STATIONS) {
    currentPlItem = config.store.lastStation;
    plCurrent.reset();
    currentScrollId = 0;
    drawPlaylist();
  }
  if (dsp_on_newmode) dsp_on_newmode(newmode);
}

void Display::drawPlayer() {
  if (clockRequest) {
    getLocalTime(&network.timeinfo);
    time();
    clockRequest = false;
  }
  meta.loop();
  title1.loop();
  if (TITLE_SIZE2 != 0) title2.loop();
}

void Display::drawVolume() {
  if (millis() - volDelay > 3000) {
    volDelay = millis();
    swichMode(PLAYER);
  }
}

void Display::resetQueue(){
  xQueueReset(displayQueue);
}

void Display::drawPlaylist() {
  char buf[PLMITEMLENGHT];
  dsp.drawPlaylist(currentPlItem, buf);
  plCurrent.setText(dsp.utf8Rus(buf, true));
}

void Display::drawNextStationNum(uint16_t num) {
  char plMenu[1][40];
  char currentItemText[40] = {0};
  config.fillPlMenu(plMenu, num, 1);
  strlcpy(currentItemText, plMenu[0], 39);
  meta.setText(dsp.utf8Rus(currentItemText, true));
  dsp.drawNextStationNum(num);
}

void Display::putRequest(requestParams_t request){
  if(displayQueue==NULL) return;
  xQueueSend(displayQueue, &request, portMAX_DELAY);
}

void Display::loop() {
  if(displayQueue==NULL) return;
  requestParams_t request;
  if(xQueueReceive(displayQueue, &request, 20)){
    switch (request.type){
      case NEWMODE: {
        swichMode((displayMode_e)request.payload);
        break;
      }
      case CLOCK: {
        clockRequest = true;
        break;
      }
      case NEWTITLE: {
        title();
        break;
      }
      case RETURNTITLE: {
        returnTile();
        break;
      }
      case NEWSTATION: {
        station();
        break;
      }
      case NEXTSTATION: {
        drawNextStationNum((displayMode_e)request.payload);
        break;
      }
      case DRAWPLAYLIST: {
        int p = request.payload ? currentPlItem + 1 : currentPlItem - 1;
        if (p < 1) p = config.store.countStation;
        if (p > config.store.countStation) p = 1;
        currentPlItem = p;
        drawPlaylist();
        break;
      }
      case DRAWVOL: {
        volume();
        break;
      }
    }
  }

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
        meta.loop();
        break;
      }
    case STATIONS: {
        plCurrent.loop();
        break;
      }
  }
  dsp.loop();
  if (dsp_on_loop) dsp_on_loop(&dsp);
}

void Display::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  dsp.centerText(text, y, fg, bg);
}

void Display::bootString(const char* text, byte y) {
  dsp.set_TextSize(1);
  dsp.centerText(text, y == 1 ? BOOTSTR_TOP1 : BOOTSTR_TOP2, TFT_LOGO, TFT_BG);
  dsp.loop(true);
}

void Display::bootLogo() {
  clear();
  dsp.drawLogo();
}

void Display::rightText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  dsp.rightText(text, y, fg, bg);
}

void Display::station() {
  meta.setText(dsp.utf8Rus(config.station.name, true));
#ifdef DEBUG_TITLES
  meta.setText(dsp.utf8Rus("Utenim adminim veniam FM", true));
#endif
  //dsp.loop(true);
  //netserver.requestOnChange(STATION, 0);
}

void Display::returnTile() {
  meta.setText(dsp.utf8Rus(config.station.name, true));
#ifdef DEBUG_TITLES
  meta.setText(dsp.utf8Rus("Utenim adminim veniam FM", true));
#endif
  meta.reset();
  //dsp.loop(true);
}

void Display::title() {
  /*
    memset(config.station.title, 0, BUFLEN);
    strlcpy(config.station.title, str, BUFLEN);
  */
  char ttl[BUFLEN / 2] = { 0 };
  char sng[BUFLEN / 2] = { 0 };
  if (strlen(config.station.title) > 0) {
    char* ici;
    if ((ici = strstr(config.station.title, " - ")) != NULL && TITLE_SIZE2 != 0) {
      strlcpy(sng, ici + 3, BUFLEN / 2);
      strlcpy(ttl, config.station.title, strlen(config.station.title) - strlen(ici) + 1);

    } else {
      strlcpy(ttl, config.station.title, BUFLEN / 2);
      sng[0] = '\0';
    }
#ifdef DEBUG_TITLES
    strlcpy(ttl, "Duis aute irure dolor in reprehenderit in voluptate velit", BUFLEN / 2);
    strlcpy(sng, "Excepteur sint occaecat cupidatat non proident", BUFLEN / 2);
#endif
    title1.setText(dsp.utf8Rus(ttl, true));
    if (TITLE_SIZE2 != 0) title2.setText(dsp.utf8Rus(sng, true));

    //dsp.loop(true);
  }
  //netserver.requestOnChange(TITLE, 0);
}

void Display::heap() {
  if (config.store.audioinfo) dsp.displayHeapForDebug();
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

void Display::checkConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    bool playing = player.mode == PLAYING;
    swichMode(LOST);
    if (playing) player.mode = STOPPED;
    WiFi.disconnect();
    ip();
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    swichMode(PLAYER);
    if (playing) player.play(config.store.lastStation);
  }
}

void Display::time(bool redraw) {
  if (dsp_before_clock) if (!dsp_before_clock(&dsp, dt)) return;
  char timeStringBuff[20] = { 0 };
  if (!dt) {
    checkConnection();
    heap();
    rssi();
  }
#ifndef TFT_FULLTIME
  if (!dt) {
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &network.timeinfo);
  } else {
    strftime(timeStringBuff, sizeof(timeStringBuff), "%H %M", &network.timeinfo);
  }
  dsp.printClock(timeStringBuff);
#else
  dsp.printClock(network.timeinfo, dt, redraw);
#endif
  dt = !dt;
  if (dsp_after_clock) dsp_after_clock(&dsp, dt);
}

void Display::volume() {
  dsp.drawVolumeBar(mode == VOL);
  //netserver.requestOnChange(VOLUME, 0);
}

#endif // DUMMYDISPLAY
