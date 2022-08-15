#include "options.h"

#include "WiFi.h"
#include "time.h"
#include "display.h"
#if VU_READY==1
#include "display_vu.h"
#endif
#include "player.h"
#include "netserver.h"
#include "network.h"

DspCore dsp;
Display display;
#ifdef USE_NEXTION
Nextion nextion;
#endif

#ifndef DUMMYDISPLAY
/******************************************************************************************************************/
void ticks() {
  network.timeinfo.tm_sec ++;
  mktime(&network.timeinfo);
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

#if WEATHER_READY==1
bool weatherRequest = false;
TaskHandle_t weatherUpdateTaskHandle;
Ticker weatherTicker;
char weatherText[254] = { 0 };
#endif

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
#ifdef USE_NEXTION
  nextion.begin();
#endif
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
#if WEATHER_READY==1
  if (DSP_MODEL == DSP_ST7735 || (DSP_MODEL == DSP_SSD1327)) {
    weatherScroll.init(5, " * ", 1, TFT_LINEHGHT * 4 + 6, 0, ORANGE, TFT_BG);
  }else if(DSP_MODEL == DSP_ILI9225){
    weatherScroll.init(5, " * ", 1, TFT_LINEHGHT * 6 + 5, 0, ORANGE, TFT_BG);
  } else {
    weatherScroll.init(5, " * ", 2, TFT_LINEHGHT * 9 + 5, 0, ORANGE, TFT_BG);
  }
#endif
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
#ifdef USE_NEXTION
    nextion.apScreen();
#endif
    return;
  }
  mode = PLAYER;
#ifdef USE_NEXTION
  nextion.putcmd("page player");
#endif
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
#ifdef USE_NEXTION
  nextion.swichMode(newmode);
#endif
  if (newmode == VOL) {
    volDelay = millis();
  }
  if (newmode == mode || network.status != CONNECTED) return;
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
  if (newmode == UPDATING) {
    dsp.frameTitle("* UPDATING *");
  }
  if (newmode == INFO || newmode == SETTINGS || newmode == TIMEZONE || newmode == WIFI) {
    dsp.frameTitle("* NEXTION *");
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
    //getLocalTime(&network.timeinfo);
    //network.timeinfo.tm_sec ++;
    //mktime(&network.timeinfo);
    time();
    clockRequest = false;
  }
  meta.loop();
  title1.loop();
  if (TITLE_SIZE2 != 0) title2.loop();
}

void Display::sendInfo(){
  if (clockRequest) {
#ifdef USE_NEXTION
  if(mode==TIMEZONE) nextion.localTime(network.timeinfo);
  if(mode==INFO)     nextion.rssi();
#endif
    clockRequest = false;
  }
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
#ifdef USE_NEXTION
  nextion.drawPlaylist(currentPlItem);
#endif
}

void Display::drawNextStationNum(uint16_t num) {
  char plMenu[1][40];
  char currentItemText[40] = {0};
  config.fillPlMenu(plMenu, num, 1, true);
  strlcpy(currentItemText, plMenu[0], 39);
  meta.setText(dsp.utf8Rus(currentItemText, true));
  dsp.drawNextStationNum(num);
#ifdef USE_NEXTION
  nextion.drawNextStationNum(num);
#endif
}

void Display::putRequest(requestParams_t request){
  if(displayQueue==NULL) return;
  xQueueSend(displayQueue, &request, portMAX_DELAY);
}

void Display::loop() {
  if(displayQueue==NULL) return;
#ifdef USE_NEXTION
  nextion.loop();
#endif
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
#if WEATHER_READY==1
        weatherScroll.loop();
#endif
        break;
      }
    case INFO:
    case TIMEZONE: {
        sendInfo();
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
    default:
        break;
  }
  dsp.loop();
  if (dsp_on_loop) dsp_on_loop(&dsp);
#if VU_READY==1
  drawVU(&dsp);
#endif
#if WEATHER_READY==1
  if (weatherRequest) {
    weatherRequest = false;
    weatherScroll.setText(dsp.utf8Rus(weatherText, true));
  }
#endif
}

void Display::centerText(const char* text, byte y, uint16_t fg, uint16_t bg) {
  dsp.centerText(text, y, fg, bg);
}

void Display::bootString(const char* text, byte y) {
  dsp.set_TextSize(1);
  dsp.centerText(text, y == 1 ? BOOTSTR_TOP1 : BOOTSTR_TOP2, TFT_LOGO, TFT_BG);
  dsp.loop(true);
#ifdef USE_NEXTION
  if(y==2) nextion.bootString(text);
#endif
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
#ifdef USE_NEXTION
  nextion.newNameset(config.station.name);
  nextion.bitrate(config.station.bitrate);
  nextion.bitratePic(ICON_NA);
#endif
#ifdef DEBUG_TITLES
  meta.setText(dsp.utf8Rus("Utenim adminim veniam FM", true));
#endif
  //dsp.loop(true);
  //netserver.requestOnChange(STATION, 0);
}

void Display::returnTile() {
  meta.setText(dsp.utf8Rus(config.station.name, true));
#ifdef USE_NEXTION
  nextion.newNameset(config.station.name);
#endif
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
#ifdef USE_NEXTION
    nextion.newTitle(config.station.title);
#endif
    //dsp.loop(true);
    if (player_on_track_change) player_on_track_change();
  }
  //netserver.requestOnChange(TITLE, 0);
}

void Display::heap() {
  if (config.store.audioinfo) dsp.displayHeapForDebug();
}

void Display::rssi() {
  int rssi = WiFi.RSSI();
  netserver.setRSSI(rssi);
  if (dsp_before_rssi) if (!dsp_before_rssi(&dsp)) return;
  char buf[20];
  sprintf(buf, "%ddBm", rssi);
  dsp.rssi(buf);
}

void Display::ip() {
  if (dsp_before_ip) if (!dsp_before_ip(&dsp)) return;
  dsp.ip(network.status == CONNECTED?WiFi.localIP().toString().c_str():WiFi.softAPIP().toString().c_str());
}

void Display::time(bool redraw) {
  if (dsp_before_clock) if (!dsp_before_clock(&dsp, dt)) return;
  char timeStringBuff[40] = { 0 };
  (void)timeStringBuff;
  if (!dt) {
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
#ifdef USE_NEXTION
  nextion.printClock(network.timeinfo);
#endif
  dt = !dt;
  if (dsp_after_clock) dsp_after_clock(&dsp, dt);
}

void Display::volume() {
  dsp.drawVolumeBar(mode == VOL);
#ifdef USE_NEXTION
  nextion.setVol(config.store.volume, mode == VOL);
#endif
  //netserver.requestOnChange(VOLUME, 0);
}

void Display::flip(){
  dsp.flip();
}

void Display::invert(){
  dsp.invert();
}
#if DSP_MODEL==DSP_NOKIA5110
void  Display::setContrast(){
  dsp.setContrast(config.store.contrast);
}
#endif // DSP_MODEL==DSP_NOKIA5110
/******************************************************************************************************************/
#endif // !DUMMYDISPLAY

#ifdef DUMMYDISPLAY
/******************************************************************************************************************/
void Display::bootString(const char* text, byte y) {
  #ifdef USE_NEXTION
  if(y==2) nextion.bootString(text);
  #endif
}
void Display::init(){
  #ifdef USE_NEXTION
  nextion.begin(true);
  #endif
}
void Display::start(bool reboot){
  #ifdef USE_NEXTION
  nextion.start();
  #endif
}
void Display::putRequest(requestParams_t request){
  #ifdef USE_NEXTION
  nextion.putRequest(request);
  #endif
}
/******************************************************************************************************************/
#endif // DUMMYDISPLAY

#ifndef DUMMYDISPLAY
#if WEATHER_READY==1

bool getForecast() {
  WiFiClient client;
  const char* host  = "api.openweathermap.org";
  
  if (!client.connect(host, 80)) {
    Serial.println("## OPENWEATHERMAP ###: connection  failed");
    return false;
  }
  char httpget[250] = {0};
  sprintf(httpget, "GET /data/2.5/weather?lat=%s&lon=%s&units=metric&lang=ru&appid=%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", config.store.weatherlat, config.store.weatherlon, config.store.weatherkey, host);
  client.print(httpget);
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 2000UL) {
      Serial.println("## OPENWEATHERMAP ###: client available timeout !");
      client.stop();
      return false;
    }
  }
  timeout = millis();
  String line = "";
  if (client.connected()) {
    while (client.available())
    {
      line = client.readStringUntil('\n');
      if (strstr(line.c_str(), "\"temp\"") != NULL) {
        client.stop();
        break;
      }
      if ((millis() - timeout) > 500)
      {
        client.stop();
        Serial.println("## OPENWEATHERMAP ###: client read timeout !");
        return false;
      }
    }
  }
  if (strstr(line.c_str(), "\"temp\"") == NULL) {
    Serial.println("## OPENWEATHERMAP ###: weather not found !");
    return false;
  }
  char *tmpe;
  char *tmps;
  const char* cursor = line.c_str();
  char desc[120], temp[20], hum[20], press[20], icon[5];

  tmps = strstr(cursor, "\"description\":\"");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  tmps += 15;
  tmpe = strstr(tmps, "\",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: description not found !"); return false;}
  strlcpy(desc, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  // "ясно","icon":"01d"}],
  tmps = strstr(cursor, "\"icon\":\"");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: icon not found !"); return false;}
  tmps += 8;
  tmpe = strstr(tmps, "\"}");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: icon not found !"); return false;}
  strlcpy(icon, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  
  tmps = strstr(cursor, "\"temp\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  tmps += 7;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: temp not found !"); return false;}
  strlcpy(temp, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  float tempf = atof(temp);

  tmps = strstr(cursor, "\"pressure\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  tmps += 11;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: pressure not found !"); return false;}
  strlcpy(press, tmps, tmpe - tmps + 1);
  cursor = tmpe + 2;
  int pressi = (float)atoi(press) / 1.333;

  tmps = strstr(cursor, "humidity\":");
  if (tmps == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  tmps += 10;
  tmpe = strstr(tmps, ",\"");
  if (tmpe == NULL) { Serial.println("## OPENWEATHERMAP ###: humidity not found !"); return false;}
  strlcpy(hum, tmps, tmpe - tmps + 1);

  Serial.printf("## OPENWEATHERMAP ###: description: %s, temp:%.1f C, pressure:%dmmHg, humidity:%s%%\n", desc, tempf, pressi, hum);
  sprintf(weatherText, "%s, %.1f C * давление: %d мм * влажность: %s%%", desc, tempf, pressi, hum);
  return true;

}

void Display::getWeather( void * pvParameters ) {
  delay(200);
  if (getForecast()) {
    weatherRequest = true;
    weatherTicker.detach();
    weatherTicker.attach(WEATHER_REQUEST_INTERVAL, display.updateWeather);
  } else {
    weatherTicker.detach();
    weatherTicker.attach(WEATHER_REQUEST_INTERVAL_FAULTY, display.updateWeather);
  }
  vTaskDelete( NULL );
}
#endif // WEATHER_READY==1

void Display::updateWeather(){
#if WEATHER_READY==1
  if(!config.store.showweather || strlen(config.store.weatherkey)==0) return;
  xTaskCreatePinnedToCore(
    getWeather,                   /* Task function. */
    "dspGetWeather1",             /* name of task. */
    1024 * 4,                     /* Stack size of task */
    NULL,                         /* parameter of the task */
    0,                            /* priority of the task */
    &weatherUpdateTaskHandle,     /* Task handle to keep track of created task */
    0);                           /* pin task to core CORE_FOR_LOOP_CONTROLS */
#endif // WEATHER_READY==1
}

void Display::showWeather(){
#if WEATHER_READY==1
  if(strlen(config.store.weatherkey)!=0 && config.store.showweather) display.updateWeather();
  if(!config.store.showweather){
    memset(weatherText, 0, sizeof(weatherText));
    weatherScroll.setText(weatherText);
  }
#endif // WEATHER_READY==1
}
/******************************************************************************************************************/
#endif // !DUMMYDISPLAY
