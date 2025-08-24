#include "options.h"
#include "rtcsupport.h"

#if RTCSUPPORTED
#include <Wire.h>

TwoWire RTCWire = TwoWire(0);
  
RTC rtc;

bool RTC::init(){
	RTCWire.begin(RTC_SDA, RTC_SCL);
	return begin(&RTCWire);
}

bool RTC::isRunning(){
#if RTC_MODULE==DS3231
	return !lostPower();
#elif RTC_MODULE==DS1307
	return isrunning();
#endif
}

void RTC::getTime(struct tm* tinfo){
	if(isRunning()){
		DateTime nowTm = now();
		tinfo->tm_sec  = nowTm.second();
		tinfo->tm_min  = nowTm.minute();
		tinfo->tm_hour = nowTm.hour();
		tinfo->tm_wday = nowTm.dayOfTheWeek();
		tinfo->tm_mday = nowTm.day();
		tinfo->tm_mon  = nowTm.month() - 1;
		tinfo->tm_year = nowTm.year() - 1900;
	}else{
		tinfo->tm_sec++;
		mktime(tinfo);
	}
}

void RTC::setTime(struct tm* tinfo){
	adjust(DateTime(tinfo->tm_year + 1900, tinfo->tm_mon + 1, tinfo->tm_mday, tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec));
}

#endif
