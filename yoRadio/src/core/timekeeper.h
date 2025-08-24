#ifndef timekeeper_h
#define timekeeper_h
#pragma once

void _syncTask(void * pvParameters);
bool _getWeather();

class TimeKeeper {
  public:
    volatile bool forceWeather;
    volatile bool forceTimeSync;
    volatile bool busy;
    char *weatherBuf;
  public:
    TimeKeeper();
    bool loop0();
    bool loop1();
    void timeTask();
    void weatherTask();
    void waitAndReturnPlayer(uint8_t time_s);
    void waitAndDo(uint8_t time_s, void (*callback)());
  private:
    uint32_t _returnPlayerTime, _doAfterTime;
    void (*_aftercallback)();
    void (*_watchdogcallback)();
    void _upRSSI();
    void _upSDPos();
    void _upClock();
    void _upScreensaver();
    void _returnPlayer();
    void _doAfterWait();
    void _doWatchDog();
    
};

extern TimeKeeper timekeeper;

#endif
