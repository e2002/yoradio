/**
 * Example of a plugin.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */
#ifndef HELLOWORLD_H
#define HELLOWORLD_H

#include "../../pluginsManager/pluginsManager.h"

class helloWorld : public Plugin {
public:
  helloWorld();
/**
 * See src/pluginsManager/pluginsManager.h for available events
 */
  void on_setup();
  void on_end_setup();
  void on_connect();
  void on_start_play();
  void on_stop_play();
  void on_track_change();
  void on_station_change();
  void on_display_queue(requestParams_t &request, bool& result);
  void on_display_player();
  void on_ticker();
  void on_btn_click(controlEvt_e &btnid);
};


#endif // HELLOWORLD_H

