#include "pluginsManager.h"

pluginsManager& getPluginsManager() {
  static pluginsManager instance;
  return instance;
}

pluginsManager& pm = getPluginsManager();

Plugin::Plugin() {
}

void Plugin::registerPlugin() {
  getPluginsManager().add(this);
}

void pluginsManager::add(Plugin* plugin) {
  plugins.push_back(plugin);
}

size_t pluginsManager::count() const {
  return plugins.size();
}

Plugin* pluginsManager::get(size_t index) {
  if (index < plugins.size()) {
    return plugins[index];
  }
  return nullptr;
}

