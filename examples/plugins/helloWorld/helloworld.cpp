/**
 * Example of a plugin.
 * To connect the plugin, copy its folder to the src/plugins directory.
 */

#include "helloworld.h"
#include "../../core/options.h"

helloWorld hellow;

helloWorld::helloWorld() {
  registerPlugin();
  log_i("Plugin is registered");
}

void helloWorld::on_setup(){
  log_i("%s called", __func__ );
}

void helloWorld::on_end_setup(){
  log_i("%s called", __func__ );
}

void helloWorld::on_connect(){
  log_i("%s called", __func__ );
}

void helloWorld::on_start_play(){
  log_i("%s called", __func__ );
}

void helloWorld::on_stop_play(){
  log_i("%s called", __func__ );
}

void helloWorld::on_track_change(){
  log_i("%s called", __func__ );
}

void helloWorld::on_station_change(){
  log_i("%s called", __func__ );
}

void helloWorld::on_display_queue(requestParams_t &request, bool& result){
  result = true;
  log_i("%s called, type=%d, payload=%d ", __func__ , request.type, request.payload);
}

void helloWorld::on_display_player(){
  log_i("%s called", __func__ );
}

void helloWorld::on_ticker(){
  log_i("%s called", __func__ );
}

void helloWorld::on_btn_click(controlEvt_e &btnid){
  log_i("%s called, btnid=%d", __func__ , btnid);
}



