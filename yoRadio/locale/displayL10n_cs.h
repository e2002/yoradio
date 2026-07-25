#ifndef dsp_full_loc
#define dsp_full_loc
#include <pgmspace.h>
/*************************************************************************************
    HOWTO:
    Copy this file to yoRadio/locale/displayL10n_custom.h
    and modify it
*************************************************************************************/
const char mon[] PROGMEM = "po";
const char tue[] PROGMEM = "út";
const char wed[] PROGMEM = "st";
const char thu[] PROGMEM = "čt";
const char fri[] PROGMEM = "pá";
const char sat[] PROGMEM = "so";
const char sun[] PROGMEM = "ne";

const char monf[] PROGMEM = "pondělí";
const char tuef[] PROGMEM = "úterý";
const char wedf[] PROGMEM = "středa";
const char thuf[] PROGMEM = "čtvrtek";
const char frif[] PROGMEM = "pátek";
const char satf[] PROGMEM = "sobota";
const char sunf[] PROGMEM = "neděle";

const char jan[] PROGMEM = "ledna";
const char feb[] PROGMEM = "února";
const char mar[] PROGMEM = "března";
const char apr[] PROGMEM = "dubna";
const char may[] PROGMEM = "května";
const char jun[] PROGMEM = "června";
const char jul[] PROGMEM = "července";
const char aug[] PROGMEM = "srpna";
const char sep[] PROGMEM = "září";
const char octt[] PROGMEM = "října";
const char nov[] PROGMEM = "listopadu";
const char decc[] PROGMEM = "prosince";

const char wn_N[]      PROGMEM = "S";
const char wn_NNE[]    PROGMEM = "SSV";
const char wn_NE[]     PROGMEM = "SV";
const char wn_ENE[]    PROGMEM = "VSV";
const char wn_E[]      PROGMEM = "V";
const char wn_ESE[]    PROGMEM = "VJV";
const char wn_SE[]     PROGMEM = "JV";
const char wn_SSE[]    PROGMEM = "JJV";
const char wn_S[]      PROGMEM = "J";
const char wn_SSW[]    PROGMEM = "JJZ";
const char wn_SW[]     PROGMEM = "JZ";
const char wn_WSW[]    PROGMEM = "ZJZ";
const char wn_W[]      PROGMEM = "Z";
const char wn_WNW[]    PROGMEM = "ZSZ";
const char wn_NW[]     PROGMEM = "SZ";
const char wn_NNW[]    PROGMEM = "SSZ";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, octt, nov, decc };
const char* const wind[]    PROGMEM = { wn_N, wn_NNE, wn_NE, wn_ENE, wn_E, wn_ESE, wn_SE, wn_SSE, wn_S, wn_SSW, wn_SW, wn_WSW, wn_W, wn_WNW, wn_NW, wn_NNW, wn_N };

const char    const_PlReady[]    PROGMEM = "[připraven]";
const char  const_PlStopped[]    PROGMEM = "[zastaveno]";
const char  const_PlConnect[]    PROGMEM = "[připojování]";
const char  const_DlgVolume[]    PROGMEM = "HLASITOST";
const char    const_DlgLost[]    PROGMEM = "* ODPOJENO *";
const char  const_DlgUpdate[]    PROGMEM = "* AKTUALIZACE *";
const char const_DlgNextion[]    PROGMEM = "NEXTION";
const char const_getWeather[]    PROGMEM = "";
const char  const_waitForSD[]    PROGMEM = "INDEX SD";

const char        apNameTxt[]    PROGMEM = "PŘÍSTUPOVÝ BOD";
const char        apPassTxt[]    PROGMEM = "HESLO";
const char       bootstrFmt[]    PROGMEM = "Připojuji se k %s";
const char        apSettFmt[]    PROGMEM = "NASTAVENÍ: HTTP://%s/";
#if EXT_WEATHER
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 pocitově: %.1f\011C \007 tlak: %d mm \007 vlhkost: %d%% \007 vítr: %.1f m/s [%s]";
#else
const char       weatherFmt[]    PROGMEM = "%s, %.1f\011C \007 tlak: %d mm \007 vlhkost: %d%%";
#endif
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "cs";       /* https://openweathermap.org/current#multi */

#endif