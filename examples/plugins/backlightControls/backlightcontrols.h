/**
 * Example of display backlight control depending on playback.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */
#ifndef BACKLIGHTCONTROLS_H
#define BACKLIGHTCONTROLS_H

#include "../../pluginsManager/pluginsManager.h"

class backlightControls : public Plugin {
public:
  backlightControls();
/**
 * See src/pluginsManager/pluginsManager.h for available events
 */
  void on_setup();
  void on_track_change();
  void on_stop_play();
};


#endif // BACKLIGHTCONTROLS_H

