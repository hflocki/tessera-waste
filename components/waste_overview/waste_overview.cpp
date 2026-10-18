#include "waste_overview.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <climits>
#include <cstring>
#include <string>

namespace esphome::waste_overview {

using tessera::Font;
namespace ui = tessera::ui;

static constexpr int MAX_EVENTS = 24;  // coming collections a tile keeps
static constexpr int MAX_KINDS = 8;    // kinds a tile shows at most
static constexpr int MAX_MATCHES = 5;  // the options match1..5 / label1..5
static constexpr uint32_t ASK_EVERY = 3600;  // seconds between two questions while nothing changes
static constexpr uint32_t RETRY_AFTER = 60;  // seconds before asking again after a failure

// ---- Days ----

// Days since 1970 of a civil date, so two dates subtract to whole days (Howard Hinnant's days_from_civil).
static int32_t civil_days(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  return era * 146097 + static_cast<int32_t>(yoe * 365 + yoe / 4 - yoe / 100 + doy) - 719468;
}

// "2026-10-09" (a whole day) or "2026-10-09T07:00:00+02:00": the day as days since 1970, and the hour (-1 for a whole
// day). False for anything else.
static bool parse_start(const char *text, int32_t &day, int8_t &hour) {
  int y = 0, m = 0, d = 0, h = -1;
  if (!text || sscanf(text, "%4d-%2d-%2d", &y, &m, &d) != 3 || m < 1 || m > 12 || d < 1 || d > 31) return false;
  if (strlen(text) >= 13 && (text[10] == 'T' || text[10] == ' ')) sscanf(text + 11, "%2d", &h);
  day = civil_days(y, static_cast<unsigned>(m), static_cast<unsigned>(d));
  hour = static_cast<int8_t>(h >= 0 && h < 24 ? h : -1);
  return true;
}

static std::string lower(const char *text) {
  std::string out(text ? text : "");
  std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
  return out;
}

// ---- What a kind of waste looks like ----

// Its icon and colour, by the words collection calendars use (German, English, Dutch): paper blue, packaging amber,
// organic green, hazardous purple, a Christmas tree green, the rest a grey bin.
struct Look {
  uint32_t icon;
  theme::Role colour;
};

static Look look_of(const std::string &low) {
  auto has = [&](std::initializer_list<const char *> words) {
    for (const char *word : words)
      if (low.find(word) != std::string::npos) return true;
    return false;
  };
  if (has({"weihnacht", "tannenbaum", "christmas", "kerstboom"})) return {0xF0531, theme::MARK_GREEN};  // tree
  if (has({"papier", "paper", "pappe", "karton", "cardboard"})) return {0xF03D3, theme::MARK_BLUE};     // package
  if (has({"gelb", "plastik", "plastic", "verpackung", "wertstoff", "pmd", "packag"}))
    return {0xF044C, theme::MARK_AMBER};  // recycle
  if (has({"bio", "grün", "gruen", "organic", "garden", "garten", "kompost", "compost", "gft", "laub", "schnitt", "strauch"}))
    return {0xF032A, theme::MARK_GREEN};  // leaf
  if (has({"schadstoff", "sonder", "hazard", "chemisch", "kca"})) return {0xF05D6, theme::MARK_PURPLE};  // alert
  if (has({"glas", "glass"})) return {0xF044C, theme::MARK_PURPLE};                                      // recycle
  return {0xF0A79, theme::MUTED};                                                                        // trash can
}

// ---- The tile ----

class OverviewTile : public tessera::Tile {
 public:
  explicit OverviewTile(WasteOverview *plugin) : plugin_(plugin) { plugin_->attach(this); }
  ~OverviewTile() override { plugin_->detach(this); }

  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    // The calendar of the inspector, or (firmware up to 0.56.0 hands a calendar tile none) the plugin's input.
    snprintf(calendar_, sizeof calendar_, "%s", c.entity && *c.entity ? c.entity : plugin_->calendar());
    days_ = c.options["days"] | 35;
    drop_after_ = c.options["drop_after"] | 10;
    match_count_ = 0;
    for (int i = 1; i <= MAX_MATCHES; ++i) {
      char key[8];
      snprintf(key, sizeof key, "match%d", i);
      const std::string word = lower(c.options[key] | "");
      if (word.empty()) continue;
      snprintf(key, sizeof key, "label%d", i);
      Match &m = matches_[match_count_++];
      snprintf(m.word, sizeof m.word, "%s", word.c_str());
      snprintf(m.label, sizeof m.label, "%s", c.options[key] | "");
    }
    // Side by side on a wide tile, one under the other on a tall one.
    across_ = width_ * 10 >= height_ * 14;
    for (int i = 0; i < MAX_KINDS; ++i) {
      Slot &s = slots_[i];
      s.icon = ui::label(c.parent, across_ ? Font::ICON : Font::ICON_SMALL, theme::MUTED);
      s.name = ui::label(c.parent, across_ ? Font::BODY_LARGE : Font::TITLE, theme::INK);
      s.when = ui::label(c.parent, Font::BODY, theme::MUTED);
      if (across_) {
        lv_obj_set_style_text_align(s.icon, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_align(s.name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_align(s.when, LV_TEXT_ALIGN_CENTER, 0);
      } else {
        lv_obj_set_style_text_align(s.when, LV_TEXT_ALIGN_RIGHT, 0);
      }
    }
    note_ = ui::label(c.parent, Font::BODY_LARGE, theme::MUTED);
    lv_obj_set_style_text_align(note_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_size(note_, width_, ui::line_height(Font::BODY_LARGE));
    lv_obj_set_pos(note_, 0, (height_ - ui::line_height(Font::BODY_LARGE)) / 2);
  }

  // The calendar's next event changed (or the tile is new): ask for the coming ones again.
  void on_state(JsonObjectConst data) override {
    const uint32_t next = data["attributes"]["start_time"] | 0u;
    if (next != next_ || (!answered_ && !asked_)) {
      next_ = next;
      ask();
    }
  }

  // The answer of calendar.get_events: {"re", "ok", "result": {"calendar.x": {"events": [{"start", "summary"}]}}}.
  void answer(JsonObjectConst message) {
    if (!asked_ || (message["re"] | 0u) != asked_) return;
    asked_ = 0;
    if (!(message["ok"] | false)) {
      failed_ = true;
      return;
    }
    failed_ = false;
    answered_ = true;
    event_count_ = 0;
    for (JsonObjectConst e : message["result"][calendar_]["events"].as<JsonArrayConst>()) {
      if (event_count_ >= MAX_EVENTS) break;
      Event &ev = events_[event_count_];
      if (!parse_start(e["start"] | "", ev.day, ev.hour)) continue;
      snprintf(ev.summary, sizeof ev.summary, "%s", e["summary"] | "");
      ++event_count_;
    }
    tessera::refresh();
  }

  void on_tick(uint32_t now) override {
    // Ask again every hour, sooner after a failure, and once more when an answer never came.
    if (now) {
      const uint32_t age = asked_at_ ? now - asked_at_ : UINT32_MAX;
      if (asked_ ? age >= RETRY_AFTER : age >= (failed_ ? RETRY_AFTER : ASK_EVERY)) ask();
    }
    if (!calendar_[0]) return paint_note(plugin_->text("choose"));
    if (!answered_) return paint_note(plugin_->text(failed_ ? "failed" : "waiting"));
    if (!now) return paint_note("");
    Kind kinds[MAX_KINDS];
    const int count = gather(now, kinds);
    if (count == 0) return paint_note(plugin_->text("none"));
    ui::set_text(note_, "");
    paint(kinds, count);
  }

  void on_theme() override {
    // Colours are set on every tick; the next one follows at once.
    ui::set_color(note_, theme::MUTED);
  }

 private:
  struct Match {
    char word[48];
    char label[48];
  };
  struct Event {
    char summary[48];
    int32_t day;
    int8_t hour;
  };
  struct Kind {
    char name[48];
    int32_t days;  // from today
    Look look;
  };
  struct Slot {
    lv_obj_t *icon{}, *name{}, *when{};
  };

  void ask() {
    asked_at_ = tessera::epoch();
    if (!calendar_[0]) return;
    JsonDocument request;
    request["ask"] = "call_service:calendar.get_events";
    request["data"]["entity_id"] = calendar_;
    request["data"]["duration"]["days"] = days_;
    asked_ = tessera::send(plugin_, request.as<JsonObjectConst>());
    failed_ = asked_ == 0;
  }

  // The next collection of each kind, soonest first. A collection of today is left out from `drop_after_` o'clock,
  // when the bin is out already, so the tile shows the one after it.
  int gather(uint32_t now, Kind *kinds) const {
    const tessera::LocalTime t = tessera::local_time(now);
    const int32_t today = civil_days(t.year, static_cast<unsigned>(t.month), static_cast<unsigned>(t.day));
    int count = 0;
    for (int i = 0; i < event_count_; ++i) {
      const Event &e = events_[i];
      if (e.day < today) continue;
      if (e.day == today && drop_after_ < 24 && t.hour >= drop_after_) continue;
      // Its kind: the first match whose word is in the event's name, under that match's name; any other event is a
      // kind of its own, under its own name.
      const std::string low = lower(e.summary);
      const char *name = nullptr;
      for (int m = 0; m < match_count_ && !name; ++m)
        if (low.find(matches_[m].word) != std::string::npos)
          name = matches_[m].label[0] ? matches_[m].label : e.summary;
      if (!name) name = e.summary;
      int k = 0;
      while (k < count && strcmp(kinds[k].name, name) != 0) ++k;
      if (k < count) {
        kinds[k].days = std::min(kinds[k].days, e.day - today);
        continue;
      }
      if (count >= MAX_KINDS) continue;
      Kind &kind = kinds[count++];
      snprintf(kind.name, sizeof kind.name, "%s", name);
      kind.days = e.day - today;
      kind.look = look_of(lower(name) + " " + low);
    }
    std::stable_sort(kinds, kinds + count, [](const Kind &a, const Kind &b) { return a.days < b.days; });
    return count;
  }

  std::string when_text(int32_t days) const {
    return days == 0 ? std::string(plugin_->text("today")) : tessera::days_text(days);
  }

  void paint_note(const std::string &text) {
    ui::set_text(note_, text);
    for (Slot &s : slots_) hide(s);
  }

  static void hide(Slot &s) {
    ui::set_text(s.icon, "");
    ui::set_text(s.name, "");
    ui::set_text(s.when, "");
  }

  void paint(const Kind *kinds, int count) {
    const int icon_h = ui::line_height(across_ ? Font::ICON : Font::ICON_SMALL);
    const int name_h = ui::line_height(across_ ? Font::BODY_LARGE : Font::TITLE);
    const int when_h = ui::line_height(Font::BODY);
    int shown;
    if (across_) {
      // As many columns as leave each at least 16 mm; the icon goes on a tile too low for it.
      shown = std::max(1, std::min(count, width_ / std::max(1, ui::mm(16))));
      const int col = width_ / shown;
      const bool with_icon = icon_h + name_h + when_h <= height_;
      const int block = (with_icon ? icon_h : 0) + name_h + when_h;
      const int top = std::max(0, (height_ - block) / 2);
      for (int i = 0; i < shown; ++i) {
        Slot &s = slots_[i];
        const int x = i * col;
        place(s.icon, x, top, col, icon_h);
        place(s.name, x + ui::px(2), top + (with_icon ? icon_h : 0), col - ui::px(4), name_h);
        place(s.when, x, top + (with_icon ? icon_h : 0) + name_h, col, when_h);
        fill(s, kinds[i], with_icon);
      }
    } else {
      // A row per kind: the icon, the name, and the days at the right.
      const int row = std::max(icon_h, name_h) + ui::px(ui::large() ? 8 : 5);
      shown = std::max(1, std::min(count, (height_ + ui::px(4)) / row));
      const int top = std::max(0, (height_ - shown * row) / 2);
      for (int i = 0; i < shown; ++i) {
        Slot &s = slots_[i];
        const std::string when = when_text(kinds[i].days);
        const int y = top + i * row;
        const int when_w = std::min(width_ / 2, ui::text_width(when, Font::BODY) + ui::px(2));
        const int icon_w = icon_h + ui::px(6);
        place(s.icon, 0, y + (row - icon_h) / 2, icon_h, icon_h);
        place(s.name, icon_w, y + (row - name_h) / 2, std::max(1, width_ - icon_w - when_w - ui::px(6)), name_h);
        place(s.when, width_ - when_w, y + (row - when_h) / 2, when_w, when_h);
        fill(s, kinds[i], true);
      }
    }
    for (int i = shown; i < MAX_KINDS; ++i) hide(slots_[i]);
  }

  void fill(Slot &s, const Kind &k, bool with_icon) {
    ui::set_text(s.icon, with_icon ? ui::icon(k.look.icon) : "");
    ui::set_color(s.icon, k.look.colour);
    ui::set_text(s.name, k.name);
    ui::set_color(s.name, theme::INK);
    ui::set_text(s.when, when_text(k.days));
    // Today and tomorrow, when the bin has to go out, in the ink of the name; later ones muted.
    ui::set_color(s.when, k.days <= 1 ? theme::INK : theme::MUTED);
  }

  static void place(lv_obj_t *obj, int x, int y, int w, int h) {
    lv_obj_set_size(obj, std::max(1, w), std::max(1, h));
    lv_obj_set_pos(obj, x, y);
  }

  WasteOverview *plugin_;
  int width_ = 0, height_ = 0;
  bool across_ = true;
  char calendar_[64]{};
  int days_ = 35, drop_after_ = 10;
  Match matches_[MAX_MATCHES]{};
  int match_count_ = 0;
  Event events_[MAX_EVENTS]{};
  int event_count_ = 0;
  uint32_t asked_ = 0, asked_at_ = 0, next_ = 0;
  bool answered_ = false, failed_ = false;
  Slot slots_[MAX_KINDS];
  lv_obj_t *note_{};
};

// ---- The plugin ----

void WasteOverview::setup() {
  add_tile("overview", [this]() { return new OverviewTile(this); });
}

void WasteOverview::on_message(JsonObjectConst message) {
  for (OverviewTile *tile : tiles_)
    if (tile) tile->answer(message);
}

void WasteOverview::attach(OverviewTile *tile) {
  for (OverviewTile *&slot : tiles_)
    if (!slot) {
      slot = tile;
      return;
    }
}

void WasteOverview::detach(OverviewTile *tile) {
  for (OverviewTile *&slot : tiles_)
    if (slot == tile) slot = nullptr;
}

}  // namespace esphome::waste_overview
