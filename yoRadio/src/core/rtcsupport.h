#ifndef rtcsupport_h
#define rtcsupport_h

#define RTCSUPPORTED (RTC_SDA!=255 && RTC_SCL!=255 && (RTC_MODULE==DS3231 || RTC_MODULE==DS1307))

#if RTCSUPPORTED
#include "RTClib.h"

#if RTC_MODULE==DS3231
	class RTC: public RTC_DS3231 {
#elif RTC_MODULE==DS1307
	class RTC: public RTC_DS1307 {
#else
	#  error ONLY DS3231 OR DS1307 MODULE SUPPORTED
#endif
	public:
		bool init();
		bool isRunning();
		void getTime(struct tm* tinfo);
		void setTime(struct tm* tinfo);
	};
extern RTC rtc;
#endif

#endif
