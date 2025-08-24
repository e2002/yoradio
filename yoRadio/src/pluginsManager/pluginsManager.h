#ifndef PLUGINSMANAGER_H
#define PLUGINSMANAGER_H

#include <Arduino.h>
#include <vector>
#include <functional>
#include "../core/common.h"

class pluginsManager;

/**
 * Plugin Class
 * 
 * Base class for creating plugins that hook into various system events.
 * To use it, inherit from Plugin and override the necessary virtual methods.
 * Examples of plugin usage can be found in the `examples/plugins` folder.
 * Place your new class in the src/plugins/<MyPlugin> directory
 */
class Plugin {
public:
  /**
   * Constructor
   * Initializes the plugin when an instance is created.
   */
  Plugin();

  /**
   * Destructor
   * Cleans up resources when the plugin is destroyed.
   */
  virtual ~Plugin() {}

  // Virtual Methods (Override in derived classes):

  /** 
   * Called at the beginning of the sketch setup process.
   * Location: setup(), yoRadio.ino
   */
  virtual void on_setup() __attribute__((weak)) {}

  /** 
   * Called at the end of the sketch setup process.
   * Location: setup(), yoRadio.ino
   */
  virtual void on_end_setup() __attribute__((weak)) {}

  /**
   * Triggered after a successful network connection.
   * Location: MyNetwork::begin(), yoRadio/src/core/network.cpp
   */
  virtual void on_connect() __attribute__((weak)) {}

  /**
   * Triggered when playback starts.
   * Location: 
   * - Player::_play(uint16_t stationId), yoRadio/src/core/player.cpp
   * - Player::browseUrl(), yoRadio/src/core/player.cpp
   */
  virtual void on_start_play() __attribute__((weak)) {}

  /**
   * Triggered when playback stops.
   * Location: Player::_stop(bool alreadyStopped), yoRadio/src/core/player.cpp
   */
  virtual void on_stop_play() __attribute__((weak)) {}

  /**
   * Triggered when the current track changes.
   * Location: Display::_title(), yoRadio/src/core/display.cpp
   */
  virtual void on_track_change() __attribute__((weak)) {}

  /**
   * Triggered when the current station changes.
   * Location: Player::loop(), yoRadio/src/core/player.cpp
   */
  virtual void on_station_change() __attribute__((weak)) {}

  /**
   * Triggered when a command is dequeued for the display.
   * Parameters:
   * - request: Reference to a requestParams_t structure (defined in yoRadio/src/core/common.h).
   * - result: Set to `false` to stop queue processing or `true` to continue.
   * Location: Display::loop(), yoRadio/src/core/display.cpp
   */
  virtual void on_display_queue(requestParams_t &request, bool &result) __attribute__((weak)) {}

  /**
   * Triggered when the player UI is displayed.
   * Location:
   * - Display::_start(), yoRadio/src/core/display.cpp
   * - Display::_swichMode(displayMode_e newmode), yoRadio/src/core/display.cpp
   */
  virtual void on_display_player() __attribute__((weak)) {}

  /**
   * Triggered once every second.
   * Location: ticks(), yoRadio/src/core/network.cpp
   */
  virtual void on_ticker() __attribute__((weak)) {}

  /**
   * Triggered when a button is clicked.
   * Parameters:
   * - btnid: Reference to the button ID (defined in yoRadio/src/core/common.h).
   * Location: onBtnClick(int id), yoRadio/src/core/controls.cpp
   */
  virtual void on_btn_click(controlEvt_e &btnid) __attribute__((weak)) {}

protected:
  /**
   * Registers the plugin with the plugin manager.
   */
  void registerPlugin();
};

class pluginsManager {
public:
  void add(Plugin* plugin);
  size_t count() const;
  Plugin* get(size_t index);
  requestParams_t *request;
  template <typename Func, typename... Args>
  inline void call_event(Func&& func, Args&&... args){
    for (auto* plugin : plugins) {
      if (plugin) {
        (plugin->*func)(std::forward<Args>(args)...);
      }
    }
  }
  inline void on_setup(){ call_event(&Plugin::on_setup); }
  inline void on_end_setup(){ call_event(&Plugin::on_end_setup); }
  inline void on_connect(){ call_event(&Plugin::on_connect); }
  inline void on_start_play(){ call_event(&Plugin::on_start_play); }
  inline void on_stop_play(){ call_event(&Plugin::on_stop_play); }
  inline void on_track_change(){ call_event(&Plugin::on_track_change); }
  inline void on_station_change(){ call_event(&Plugin::on_station_change); }
  inline void on_display_queue(requestParams_t &request, bool& result){ call_event(&Plugin::on_display_queue, request, result); }
  inline void on_display_player(){ call_event(&Plugin::on_display_player); }
  inline void on_ticker(){ call_event(&Plugin::on_ticker); }
  inline void on_btn_click(controlEvt_e &btnid){ call_event(&Plugin::on_btn_click, btnid); }
private:
  std::vector<Plugin*> plugins;
};

extern pluginsManager pm;

#endif // PLUGINSMANAGER_H

