from typing_extensions import Required
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.util import Registry
from esphome.components import uart, binary_sensor
from esphome.components.uart import UARTComponent
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    DEVICE_CLASS_OCCUPANCY,
)

AUTO_LOAD = ["binary_sensor"]

DEPENDENCIES = ["uart"]

UART_ID = "uart_id"

CONF_HUB = "hub"

CONF_TARGETS = "targets"
CONF_TARGET = "target"
CONF_DEBUG = "debug"
CONF_USE_FAST_OFF = "fast_off_detection"
CONF_FLIP_X_AXIS = "flip_x_axis"
CONF_OCCUPANCY = "occupancy"

ld2450_ns = cg.esphome_ns.namespace("ld2450")
LD2450 = ld2450_ns.class_("LD2450", cg.Component, uart.UARTDevice)

TARGET_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_NAME): cv.string,
        cv.Optional(CONF_DEBUG, default=False): cv.boolean,
    }
)

TARGET_REGISTRY = Registry()
validate_targets = cv.validate_registry(CONF_TARGETS, TARGET_REGISTRY)


def register_target():
    """Register targets on the target registry."""
    return TARGET_REGISTRY.register(
        CONF_TARGET, ld2450_ns.class_("Target", cg.Component), TARGET_SCHEMA
    )


def validate_config(config):
    """Validate the LD2450 config."""

    if CONF_TARGETS not in config:
        return config

    # Ensure number of targets is bounded
    if len(config[CONF_TARGETS]) == 0:
        raise cv.Invalid("No targets specified.")

    if len(config[CONF_TARGETS]) > 3:
        raise cv.Invalid("Too many targets defined (max 3).")

    return config


CONFIG_SCHEMA = cv.All(
    uart.UART_DEVICE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LD2450),
            cv.Required(UART_ID): cv.use_id(UARTComponent),
            cv.Optional(CONF_NAME, default="LD2450"): cv.string,
            cv.Optional(CONF_TARGETS): validate_targets,
            cv.Optional(CONF_FLIP_X_AXIS, default=False): cv.boolean,
            cv.Optional(CONF_USE_FAST_OFF, default=False): cv.boolean,
            cv.Optional(CONF_OCCUPANCY): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_OCCUPANCY
            ),
        }
    ),
    validate_config,
)


def to_code(config):
    """Code generation for the LD2450 component."""
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_flip_x_axis(config[CONF_FLIP_X_AXIS]))
    cg.add(var.set_fast_off_detection(config[CONF_USE_FAST_OFF]))

    # process target list
    if targets_config := config.get(CONF_TARGETS):
        targets = yield cg.build_registry_list(TARGET_REGISTRY, targets_config)

        # Register target on controller
        for target in targets:
            cg.add(var.register_target(target))

    if occupancy_config := config.get(CONF_OCCUPANCY):
        occupancy_binary_sensor = yield binary_sensor.new_binary_sensor(
            occupancy_config
        )
        cg.add(var.set_occupancy_binary_sensor(occupancy_binary_sensor))


@register_target()
def target_to_code(config, target_id):
    """Code generation for targets within the target list."""
    var = cg.new_Pvariable(target_id)
    yield cg.register_component(var, config)

    if CONF_NAME in config:
        cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_debugging(config[CONF_DEBUG]))
