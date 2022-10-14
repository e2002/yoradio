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
const char oct[] PROGMEM = "october";
const char nov[] PROGMEM = "november";
const char dec[] PROGMEM = "december";

const char* const dow[]     PROGMEM = { sun, mon, tue, wed, thu, fri, sat };
const char* const dowf[]    PROGMEM = { sunf, monf, tuef, wedf, thuf, frif, satf };
const char* const mnths[]   PROGMEM = { jan, feb, mar, apr, may, jun, jul, aug, sep, oct, nov, dec };

const char    const_PlReady[]    PROGMEM = "[ready]";
const char  const_PlStopped[]    PROGMEM = "[stopped]";
const char  const_PlConnect[]    PROGMEM = "[connecting]";
const char  const_DlgVolume[]    PROGMEM = "VOLUME";
const char    const_DlgLost[]    PROGMEM = "* LOST *";
const char  const_DlgUpdate[]    PROGMEM = "* UPDATING *";
const char const_DlgNextion[]    PROGMEM = "* NEXTION *";
const char const_getWeather[]    PROGMEM = "";

const char        apNameTxt[]    PROGMEM = "AP NAME";
const char        apPassTxt[]    PROGMEM = "PASSWORD";
const char       bootstrFmt[]    PROGMEM = "Trying to %s";
const char        apSettFmt[]    PROGMEM = "SETTINGS PAGE ON: HTTP://%s/";
const char       weatherFmt[]    PROGMEM = "%s, %.1f C * pressure: %d mm * humidity: %s%%";
const char     weatherUnits[]    PROGMEM = "metric";   /* standard, metric, imperial */
const char      weatherLang[]    PROGMEM = "en";       /* https://openweathermap.org/current#multi */

#endif
