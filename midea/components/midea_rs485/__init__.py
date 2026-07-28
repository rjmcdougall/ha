import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, select, number, switch
from esphome.const import (
    CONF_ID,
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

CODEOWNERS = []
AUTO_LOAD = ["sensor"]
DEPENDENCIES = []

midea_rs485_ns = cg.esphome_ns.namespace("midea_rs485")
MideaRS485Component = midea_rs485_ns.class_(
    "MideaRS485Component", cg.PollingComponent
)

CONF_DI_PIN = "di_pin"
CONF_RO_PIN = "ro_pin"
CONF_DE_PIN = "de_pin"
CONF_MASTER_ID = "master_id"
CONF_SLAVE_ID = "slave_id"
CONF_MASTER_SEND_TIME = "master_send_time"
CONF_SLAVE_TIMEOUT = "slave_timeout"
CONF_T1_TEMP = "t1_temp"
CONF_T2A_TEMP = "t2a_temp"
CONF_T2B_TEMP = "t2b_temp"
CONF_T3_TEMP = "t3_temp"
CONF_NOT_RESPONDING = "not_responding"
CONF_MODE_SELECT = "mode_select"
CONF_FAN_MODE_SELECT = "fan_mode_select"
CONF_SET_TEMP = "set_temp"
CONF_AUX_HEAT = "aux_heat"
CONF_ECHO_SLEEP = "echo_sleep"
CONF_SWING = "swing"
CONF_VENT = "vent"
CONF_LOCK = "lock"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MideaRS485Component),
        cv.Optional(CONF_DI_PIN, default=16): cv.int_range(0, 39),
        cv.Optional(CONF_RO_PIN, default=17): cv.int_range(0, 39),
        cv.Optional(CONF_DE_PIN, default=4): cv.int_range(0, 39),
        cv.Optional(CONF_MASTER_ID, default=0): cv.int_range(0, 255),
        cv.Optional(CONF_SLAVE_ID, default=0): cv.int_range(0, 255),
        cv.Optional(CONF_MASTER_SEND_TIME, default=40): cv.int_range(0, 255),
        cv.Optional(CONF_SLAVE_TIMEOUT, default=100): cv.int_range(0, 255),
        cv.Optional(CONF_T1_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_T2A_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_T2B_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_T3_TEMP): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_NOT_RESPONDING): sensor.sensor_schema(
            accuracy_decimals=0,
        ),
        cv.Optional(CONF_MODE_SELECT): cv.use_id(select.Select),
        cv.Optional(CONF_FAN_MODE_SELECT): cv.use_id(select.Select),
        cv.Optional(CONF_SET_TEMP): cv.use_id(number.Number),
        cv.Optional(CONF_AUX_HEAT): cv.use_id(switch.Switch),
        cv.Optional(CONF_ECHO_SLEEP): cv.use_id(switch.Switch),
        cv.Optional(CONF_SWING): cv.use_id(switch.Switch),
        cv.Optional(CONF_VENT): cv.use_id(switch.Switch),
        cv.Optional(CONF_LOCK): cv.use_id(switch.Switch),
    }
).extend(cv.polling_component_schema("10s"))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_pins(config[CONF_DI_PIN], config[CONF_RO_PIN], config[CONF_DE_PIN]))
    cg.add(var.set_ids(config[CONF_MASTER_ID], config[CONF_SLAVE_ID]))
    cg.add(var.set_timing(config[CONF_MASTER_SEND_TIME], config[CONF_SLAVE_TIMEOUT]))

    for key, setter in [
        (CONF_T1_TEMP, "set_t1_sensor"),
        (CONF_T2A_TEMP, "set_t2a_sensor"),
        (CONF_T2B_TEMP, "set_t2b_sensor"),
        (CONF_T3_TEMP, "set_t3_sensor"),
        (CONF_NOT_RESPONDING, "set_not_responding_sensor"),
    ]:
        if key in config:
            sens = await sensor.new_sensor(config[key])
            cg.add(getattr(var, setter)(sens))

    for key, setter in [
        (CONF_MODE_SELECT, "set_mode_select"),
        (CONF_FAN_MODE_SELECT, "set_fan_mode_select"),
        (CONF_SET_TEMP, "set_temp_number"),
        (CONF_AUX_HEAT, "set_aux_heat_switch"),
        (CONF_ECHO_SLEEP, "set_echo_sleep_switch"),
        (CONF_SWING, "set_swing_switch"),
        (CONF_VENT, "set_vent_switch"),
        (CONF_LOCK, "set_lock_switch"),
    ]:
        if key in config:
            obj = await cg.get_variable(config[key])
            cg.add(getattr(var, setter)(obj))

    cg.add_library(
        name="ESP32_Midea_RS485",
        version=None,
        repository="https://github.com/Bunicutz/ESP32_Midea_RS485.git",
    )
