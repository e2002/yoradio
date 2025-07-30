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
const char octt[] PROGMEM = "октября";
const char nov[] PROGMEM = "ноября";
const char decc[] PROGMEM = "декабря";

const char wn_N[]      PROGMEM = "СЕВ";
const char wn_NNE[]    PROGMEM = "ССВ";
const char wn_NE[]     PROGMEM = "СВ";
const char wn_ENE[]    PROGMEM = "ВСВ";
const char wn_E[]      PROGMEM = "ВОСТ";
const char wn_ESE[]    PROGMEM = "ВЮВ";
const char wn_SE[]     PROGMEM = "ЮВ";
const char wn_SSE[]    PROGMEM = "ЮЮВ";
const char wn_S[]      PROGMEM = "ЮЖ";
const char wn_SSW[]    PROGMEM = "ЮЮЗ";
const char wn_SW[]     PROGMEM = "ЮЗ";
const char wn_WSW[]    PROGMEM = "ЗЮЗ";
const char wn_W[]      PROGMEM = "ЗАП";
const char wn_WNW[]    PROGMEM = "ЗСЗ";
const char wn_NW[]     PROGMEM = "СЗ";
const char wn_NNW[]    PROGMEM = "ССЗ";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, octt, nov, decc };
const char* const wind[]    PROGMEM = { wn_N, wn_NNE, wn_NE, wn_ENE, wn_E, wn_ESE, wn_SE, wn_SSE, wn_S, wn_SSW, wn_SW, wn_WSW, wn_W, wn_WNW, wn_NW, wn_NNW, wn_N };

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
const char        apPassTxt[]    PROGMEM = "ПАРОЛЬ";
const char       bootstrFmt[]    PROGMEM = "Соединяюсь с %s";
const char        apSettFmt[]    PROGMEM = "НАСТРОЙКИ: HTTP://%s/";
#if EXT_WEATHER
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 ощущается: %.1f\011C \007 давление: %d мм \007 влажность: %d%% \007 ветер: %.1f м/с [%s]";
#else
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 давление: %d mm \007 влажность: %d%%";
#endif
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "ru";       /* https://openweathermap.org/current#multi */

#endif
