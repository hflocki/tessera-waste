#pragma once
#include <cstdio>
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::waste_overview {

class OverviewTile;

// The plugin: one tile type, "overview". The app's answers to the tiles' questions reach the plugin (on_message),
// which hands them to every tile on the glass; each keeps only its own.
class WasteOverview : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void on_message(JsonObjectConst message) override;

  void set_calendar(const char *entity) { snprintf(calendar_, sizeof calendar_, "%s", entity ? entity : ""); }
  const char *calendar() const { return calendar_; }

  void attach(OverviewTile *tile);
  void detach(OverviewTile *tile);

 protected:
  static constexpr int MAX_TILES = 8;
  OverviewTile *tiles_[MAX_TILES]{};
  char calendar_[64]{};
};

}  // namespace esphome::waste_overview
