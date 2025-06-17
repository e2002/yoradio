#ifndef dsp_full_loc
#define dsp_full_loc
#include <pgmspace.h>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
const char mon[] PROGMEM = "пн";
const char tue[] PROGMEM = "вт";
const char wed[] PROGMEM = "ср";
const char thu[] PROGMEM = "чт";
const char fri[] PROGMEM = "пт";
const char sat[] PROGMEM = "сб";
const char sun[] PROGMEM = "вс";

const char monf[] PROGMEM = "понедельник";
const char tuef[] PROGMEM = "вторник";
const char wedf[] PROGMEM = "среда";
const char thuf[] PROGMEM = "четверг";
const char frif[] PROGMEM = "пятница";
const char satf[] PROGMEM = "суббота";
const char sunf[] PROGMEM = "воскресенье";

const char jan[] PROGMEM = "января";
const char feb[] PROGMEM = "февраля";
const char mar[] PROGMEM = "марта";
const char apr[] PROGMEM = "апреля";
const char may[] PROGMEM = "мая";
const char jun[] PROGMEM = "июня";
const char jul[] PROGMEM = "июля";
const char aug[] PROGMEM = "августа";
const char sep[] PROGMEM = "сентября";
const char ocb[] PROGMEM = "октября";
const char nov[] PROGMEM = "ноября";
const char dcb[] PROGMEM = "декабря";

const char wn_N[]      PROGMEM = "Северный";
const char wn_NE[]     PROGMEM = "Северо-Восточный";
const char wn_E[]      PROGMEM = "Восточный";
const char wn_SE[]     PROGMEM = "Юго-Восточный";
const char wn_S[]      PROGMEM = "Южный";
const char wn_SW[]     PROGMEM = "Юго-Западный";
const char wn_W[]      PROGMEM = "Западный";
const char wn_NW[]     PROGMEM = "Северо-Западный";
const char prv[]    PROGMEM = ", порывы ";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, ocb, nov, dcb };
const char* const wind[]    PROGMEM = { wn_N, wn_NE, wn_NE, wn_E, wn_E, wn_SE, wn_SE, wn_S, wn_S, wn_SW, wn_SW, wn_W, wn_W, wn_NW, wn_NW, wn_N, wn_N };

const char    const_PlReady[]    PROGMEM = "[готов]";
const char  const_PlStopped[]    PROGMEM = "[остановлено]";
const char  const_PlConnect[]    PROGMEM = "[соединение]";
const char  const_DlgVolume[]    PROGMEM = "ГРОМКОСТЬ";
const char    const_DlgLost[]    PROGMEM = "ОТКЛЮЧЕНО";
const char  const_DlgUpdate[]    PROGMEM = "ОБНОВЛЕНИЕ";
const char const_DlgNextion[]    PROGMEM = "NEXTION";
const char const_getWeather[]    PROGMEM = "";
const char  const_waitForSD[]    PROGMEM = "ИНДЕКС SD";

const char        apNameTxt[]    PROGMEM = "ТОЧКА ДОСТУПА";
const char        apPassTxt[]    PROGMEM = "БЕЗ ПАРОЛЯ";
const char       bootstrFmt[]    PROGMEM = "Соединяюсь с %s";
const char        apSettFmt[]    PROGMEM = "НАСТРОЙКИ: HTTP://%s/";
#if EXT_WEATHER
const char       weatherFmt[]    PROGMEM = "%s, темп.: %+.1f\011C (ощущ.как %+.0f\011C) \007 давл.: %d мм \007 влаж.: %s%% \007 ветер %s %.0f%s м/с (м.ст. %s)";
#else
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 давление: %d mm \007 влажность: %s%%";
#endif
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "ru";       /* https://openweathermap.org/current#multi */

#endif
