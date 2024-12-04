/**
 * Example of esp32 deep sleep when playback is stopped.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */
#ifndef DEEPSLEEP_H
#define DEEPSLEEP_H

#include "../../pluginsManager/pluginsManager.h"

class deepSleep : public Plugin {
public:
  deepSleep();
/**
 * See src/pluginsManager/pluginsManager.h for available events
 */
  void on_setup();
  void on_start_play();
  void on_stop_play();
};


#endif // DEEPSLEEP_H

