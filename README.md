# Waste overview

Every kind of waste of a calendar in Home Assistant side by side on one tile: its icon in its colour, its name, and how
many days until it goes out next ("Tomorrow", "In 9 days"), the soonest first. It is the idea of the Lovelace
[trash-card](https://github.com/idaho/hassio-trash-card), drawn on a Tessera screen.

On a wide tile the kinds stand side by side, as many as fit; on a tall one they are rows, with the days at the right.

The icon and colour follow the words calendars use: paper blue, the yellow bag and packaging amber, organic waste
green, hazardous waste purple, a Christmas tree green, everything else a grey bin. German, English and Dutch words
work.

## Set up

1. Have a calendar with the collection days in Home Assistant. [Waste Collection Schedule](https://github.com/mampfes/hacs_waste_collection_schedule)
   makes one for most councils; a Local Calendar with the days typed in works too.
2. In Tessera, open **Plugins**, **Add with a link**, and paste the link of this repository. Add the plugin to a
   screen and choose the calendar under **Waste calendar**. The screen builds once.
3. In **Layout**, place the **Waste overview** tile from the library's Plugins group (4 x 1 shows four kinds side by
   side) and choose the calendar in its inspector.
4. That is all: every event name of the calendar is a kind of its own. To group or rename them, fill in the kinds
   below.

## Options

- **Days ahead**: how far ahead to look, 35 days by default. A kind collected less often than that does not show.
- **Today's collection until**: from this hour on (10 by default) a collection of today makes way for the next one of
  its kind, as the bin is out already. 24 keeps it all day.
- **Kind 1 to 5, word and name**: renames and groups kinds. Every event whose name holds the word shows under the
  name: `Rest` with the name `Restmüll` turns "Restmülltonne 2-wöchentlich" into "Restmüll", and `Gelb` with
  `Gelber Sack` puts "Gelbe Tonne" and "Gelber Sack" together. Every other event keeps its own name.

## Coming collections

A tap on the tile opens a card with the coming collections of five weeks: each with its icon, its name (renamed as on
the tile), its date and how far away.

## Good to know

- The tile asks Home Assistant for the coming events through Tessera (`calendar.get_events`): when it appears, when
  the calendar's next event changes, and every hour.
- Up to eight kinds and 24 coming collections; an answer of Home Assistant is at most about 3 KB, so a calendar with
  long descriptions may lose its last collections. Fewer **Days ahead** helps then.
- Works on every board, the CYD included.

## A hobby project

This plugin is a private, non-commercial hobby project. I build and maintain it in my spare time and share it
free of charge as open source under the [MIT License](LICENSE). It is free and will stay free: there is no paid
version, no paid feature and no paid support.

Bug reports and ideas are welcome, but please keep in mind:

- There is no support and no guaranteed response time. I answer when I find the time, which can take days or weeks.
- Not every request gets built. I add what fits the plugin and what I can keep working over time.
- Pull requests are welcome; for a larger change, open an issue first so we can agree on the way.

### No warranty, no liability

The plugin is provided "as is", without any warranty and without guaranteed support, updates or fixes. You use it at
your own risk. To the extent permitted by law, I accept no liability for any damage or data loss resulting from its
use. The full terms are in the [MIT License](LICENSE).

### Buy me a coffee

If the plugin is useful to you and you would like to say thanks, you are welcome to buy me a coffee. A donation is
entirely voluntary: it is a thank-you, not a payment. It does not buy support, features, a response time or any other
service, and the plugin stays exactly the same for everyone, whether they donate or not.

[![Buy me a coffee](https://img.buymeacoffee.com/button-api/?text=Buy%20me%20a%20coffee&emoji=&slug=hflocki&button_colour=FFDD00&font_colour=000000&font_family=Cookie&outline_colour=000000&coffee_colour=ffffff)](https://www.buymeacoffee.com/hflocki)
