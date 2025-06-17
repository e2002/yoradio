#ifndef dsp_full_loc
#define dsp_full_loc
#include <pgmspace.h>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
const char mon[] PROGMEM = "mo";
const char tue[] PROGMEM = "tu";
const char wed[] PROGMEM = "we";
const char thu[] PROGMEM = "th";
const char fri[] PROGMEM = "fr";
const char sat[] PROGMEM = "sa";
const char sun[] PROGMEM = "su";

const char monf[] PROGMEM = "monday";
const char tuef[] PROGMEM = "tuesday";
const char wedf[] PROGMEM = "wednesday";
const char thuf[] PROGMEM = "thursday";
const char frif[] PROGMEM = "friday";
const char satf[] PROGMEM = "saturday";
const char sunf[] PROGMEM = "sunday";

const char jan[] PROGMEM = "january";
const char feb[] PROGMEM = "february";
const char mar[] PROGMEM = "march";
const char apr[] PROGMEM = "april";
const char may[] PROGMEM = "may";
const char jun[] PROGMEM = "june";
const char jul[] PROGMEM = "july";
const char aug[] PROGMEM = "august";
const char sep[] PROGMEM = "september";
const char ocb[] PROGMEM = "october";
const char nov[] PROGMEM = "november";
const char dcb[] PROGMEM = "december";

const char wn_N[]      PROGMEM = "North";
const char wn_NE[]     PROGMEM = "North-Eastern";
const char wn_E[]      PROGMEM = "Eastern";
const char wn_SE[]     PROGMEM = "South-Eastern";
const char wn_S[]      PROGMEM = "Southern";
const char wn_SW[]     PROGMEM = "South-Western";
const char wn_W[]      PROGMEM = "Western";
const char wn_NW[]     PROGMEM = "North-West";
const char prv[]    PROGMEM = ", gusts ";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, ocb, nov, dcb };
const char* const wind[]    PROGMEM = { wn_N, wn_NE, wn_NE, wn_E, wn_E, wn_SE, wn_SE, wn_S, wn_S, wn_SW, wn_SW, wn_W, wn_W, wn_NW, wn_NW, wn_N, wn_N };

const char    const_PlReady[]    PROGMEM = "[ready]";
const char  const_PlStopped[]    PROGMEM = "[stopped]";
const char  const_PlConnect[]    PROGMEM = "[connecting]";
const char  const_DlgVolume[]    PROGMEM = "VOLUME";
const char    const_DlgLost[]    PROGMEM = "* LOST *";
const char  const_DlgUpdate[]    PROGMEM = "* UPDATING *";
const char const_DlgNextion[]    PROGMEM = "* NEXTION *";
const char const_getWeather[]    PROGMEM = "";
const char  const_waitForSD[]    PROGMEM = "INDEX SD";

const char        apNameTxt[]    PROGMEM = "AP NAME";
const char        apPassTxt[]    PROGMEM = "W/O PASSWORD";
const char       bootstrFmt[]    PROGMEM = "Wi-fi: %s";
const char        apSettFmt[]    PROGMEM = "SETTINGS PAGE ON: HTTP://%s/";
#if EXT_WEATHER
const char       weatherFmt[]    PROGMEM = "%s, temp: %+.1f\011C \007 (feels like: %+.0f\011C) \007 pressure: %d mm \007 humidity: %s%% \007 wind: %s %.0f%s m/s (m.st. %s)";
#else
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 pressure: %d mm \007 humidity: %s%%";
#endif
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "en";       /* https://openweathermap.org/current#multi */

#endif
