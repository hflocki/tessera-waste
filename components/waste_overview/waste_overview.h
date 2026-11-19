#pragma once
#include <cstdio>
#include "esphome/core/component.h"
#include "esphome/components/smart_display/plugin_api.h"

namespace esphome::waste_overview {

class OverviewTile;
class UpcomingCard;

// Room for a whole entity id: Home Assistant allows up to 255 characters, and calendars of Waste Collection Schedule
// are often longer than 63 (calendar.waste_collection_schedule_<the service's name>).
static constexpr size_t ENTITY_ID_LEN = 256;

// A word of the inspector that renames and groups kinds (Rest: Restmüll): the word folded for comparing, and the name.
struct KindWord {
  char word[48];
  char label[48];
};

// What the card opened from a tile needs of that tile: its calendar, its words and from which hour today's
// collection makes way.
struct CardSource {
  char calendar[ENTITY_ID_LEN];
  KindWord words[5];
  int word_count;
  int drop_after;
};

// The plugin: a tile type ("overview") and a card with the coming collections ("upcoming"). The app's answers to the
// questions reach the plugin (on_message), which hands them to every tile on the glass and the open card; each keeps
// only its own.
class WasteOverview : public Component, public tessera::Plugin {
 public:
  void setup() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void on_message(JsonObjectConst message) override;

  void set_calendar(const char *entity) { snprintf(calendar_, sizeof calendar_, "%s", entity ? entity : ""); }
  const char *calendar() const { return calendar_; }

  void attach(OverviewTile *tile);
  void detach(OverviewTile *tile);

  CardSource card_source{};            // set by the tile that opens the card
  UpcomingCard *open_card_ = nullptr;  // the card that is open, to hand it its answer

 protected:
  static constexpr int MAX_TILES = 8;
  OverviewTile *tiles_[MAX_TILES]{};
  char calendar_[ENTITY_ID_LEN]{};
};

}  // namespace esphome::waste_overview
