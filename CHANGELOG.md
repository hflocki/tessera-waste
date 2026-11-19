# Changelog

## 1.2.4
- The card with the coming collections makes room for how far away in the screen's language, so "In 12 Tagen" is no
  longer cut to "In 12 Ta…".

## 1.2.3
- A calendar whose entity id is longer than 63 characters (often one of Waste Collection Schedule) is asked for by its
  whole id: it was cut, so Home Assistant refused the question and the tile said "The calendar does not answer" or
  "Nothing planned" (#1, #3).

## 1.2.2
- Icons on a low wide tile (4x1): the small icon above the name where it fits, else in front of the name, instead of
  none at all (#2).
- The events are found when Home Assistant's answer names the calendar differently from its entity id, so the tile no
  longer says "Nothing planned" for a calendar with events (#3).
- The log says why the calendar does not answer (the error of Home Assistant or the app) and which calendars an answer
  held when none matched (#1).
- A change of the calendar's next event asks for the coming collections at once again; the start time comes as a text.
## 1.2.1
- Amber and yellow words and icons take the ink on the light look, where they were too faint to read; the dark look keeps its colours.

## 1.2.0
- A tap on the tile opens the coming collections of five weeks, with their date and how far away, named as on
  the tile.

## 0.1.2
- A word with an umlaut (Restmülltonne) now finds its events however the calendar writes the umlaut.

## 0.1.1
- Kind 1 to 5 only rename and group now: events that match none of them still show, under their own name.

## 0.1.0
- First version: every kind of waste of a calendar side by side, each with its icon, its name and the days until it
  goes out next.
