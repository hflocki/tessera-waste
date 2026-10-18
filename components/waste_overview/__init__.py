"""Waste overview: every kind of waste of a calendar side by side, with how many days until it goes out next.

The tile asks Home Assistant for the coming events through the app (calendar.get_events) and draws them
(waste_overview.cpp). register_plugin() takes the id, version, tile memory and the screen's texts from the manifest and
translations/ beside this folder.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import smart_display
from esphome.const import CONF_ID

DEPENDENCIES = ["smart_display"]

CONF_CALENDAR = "calendar"

waste_overview_ns = cg.esphome_ns.namespace("waste_overview")
WasteOverview = waste_overview_ns.class_("WasteOverview", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(WasteOverview),
        cv.Required(CONF_CALENDAR): cv.entity_id,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await smart_display.register_plugin(var, __file__)
    cg.add(var.set_calendar(config[CONF_CALENDAR]))
