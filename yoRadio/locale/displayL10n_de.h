#ifndef dsp_full_loc
#define dsp_full_loc
#include <pgmspace.h>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
const char mon[] PROGMEM = "Mo";
const char tue[] PROGMEM = "Di";
const char wed[] PROGMEM = "Mi";
const char thu[] PROGMEM = "Do";
const char fri[] PROGMEM = "Fr";
const char sat[] PROGMEM = "Sa";
const char sun[] PROGMEM = "So";

const char monf[] PROGMEM = "Montag";
const char tuef[] PROGMEM = "Dienstag";
const char wedf[] PROGMEM = "Mittwoch";
const char thuf[] PROGMEM = "Donnerstag";
const char frif[] PROGMEM = "Freitag";
const char satf[] PROGMEM = "Samstag";
const char sunf[] PROGMEM = "Sonntag";

const char jan[] PROGMEM = "Januar";
const char feb[] PROGMEM = "Februar";
const char mar[] PROGMEM = "März";
const char apr[] PROGMEM = "April";
const char may[] PROGMEM = "Mai";
const char jun[] PROGMEM = "Juni";
const char jul[] PROGMEM = "Juli";
const char aug[] PROGMEM = "August";
const char sep[] PROGMEM = "Septempber";
const char oct[] PROGMEM = "Oktober";
const char nov[] PROGMEM = "November";
const char dec[] PROGMEM = "Dezember";

const char wn_N[]      PROGMEM = "NORD";
const char wn_NNE[]    PROGMEM = "NNO";
const char wn_NE[]     PROGMEM = "NO";
const char wn_ENE[]    PROGMEM = "ONO";
const char wn_E[]      PROGMEM = "OST";
const char wn_ESE[]    PROGMEM = "OSO";
const char wn_SE[]     PROGMEM = "SO";
const char wn_SSE[]    PROGMEM = "SSO";
const char wn_S[]      PROGMEM = "SÜD";
const char wn_SSW[]    PROGMEM = "SSW";
const char wn_SW[]     PROGMEM = "SW";
const char wn_WSW[]    PROGMEM = "WSW";
const char wn_W[]      PROGMEM = "WEST";
const char wn_WNW[]    PROGMEM = "WNW";
const char wn_NW[]     PROGMEM = "NW";
const char wn_NNW[]    PROGMEM = "NNW";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec };
const char* const wind[]    PROGMEM = { wn_N, wn_NNE, wn_NE, wn_ENE, wn_E, wn_ESE, wn_SE, wn_SSE, wn_S, wn_SSW, wn_SW, wn_WSW, wn_W, wn_WNW, wn_NW, wn_NNW, wn_N };

const char    const_PlReady[]    PROGMEM = "[ready]";
const char  const_PlStopped[]    PROGMEM = "[stopped]";
const char  const_PlConnect[]    PROGMEM = "[connecting]";
const char  const_DlgVolume[]    PROGMEM = "VOLUME";
const char    const_DlgLost[]    PROGMEM = "* LOST *";
const char  const_DlgUpdate[]    PROGMEM = "* UPDATING *";
const char const_DlgNextion[]    PROGMEM = "* NEXTION *";
const char const_getWeather[]    PROGMEM = "* NO WEATHER DATA *";
const char  const_waitForSD[]    PROGMEM = "INDEX SD";

const char        apNameTxt[]    PROGMEM = "AP NAME";
const char        apPassTxt[]    PROGMEM = "PASSWORD";
const char       bootstrFmt[]    PROGMEM = "Trying to %s";
const char        apSettFmt[]    PROGMEM = "SETTINGS PAGE ON: HTTP://%s/";
#if EXT_WEATHER
const char       weatherFmt[]    PROGMEM = "%s,* %.1f\011C \007 gefühlt: %.1f\011C \007 Druck: %d hPa \007 rel Luftfeuchte: %s%% \007 Wind: %.1f m/s aus [%s]";
#else
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 Druck: %d hPa \007 rel Luftfeuchte: %s%%";
#endif
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "de";       /* https://openweathermap.org/current#multi */

#endif
