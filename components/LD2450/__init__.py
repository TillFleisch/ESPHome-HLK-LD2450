from typing_extensions import Required
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, binary_sensor, number
from esphome.components.uart import UARTComponent
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_STEP,
    CONF_RESTORE_VALUE,
    CONF_INITIAL_VALUE,
    CONF_UNIT_OF_MEASUREMENT,
    UNIT_METER,
    DEVICE_CLASS_OCCUPANCY,
)

AUTO_LOAD = ["binary_sensor", "number"]

DEPENDENCIES = ["uart"]

UART_ID = "uart_id"

CONF_HUB = "hub"

CONF_TARGETS = "targets"
CONF_TARGET = "target"
CONF_DEBUG = "debug"
CONF_USE_FAST_OFF = "fast_off_detection"
CONF_FLIP_X_AXIS = "flip_x_axis"
CONF_OCCUPANCY = "occupancy"
CONF_MAX_DISTANCE = "max_detection_distance"
CONF_MAX_DISTANCE_MARGIN = "max_distance_margin"


ld2450_ns = cg.esphome_ns.namespace("ld2450")
LD2450 = ld2450_ns.class_("LD2450", cg.Component, uart.UARTDevice)
Target = ld2450_ns.class_("Target", cg.Component)
MaxDistanceNumber = ld2450_ns.class_("MaxDistanceNumber", cg.Component)

TARGET_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_TARGET): cv.Optional(
            {
                cv.GenerateID(): cv.declare_id(Target),
                cv.Optional(CONF_NAME): cv.string_strict,
                cv.Optional(CONF_DEBUG, default=False): cv.boolean,
            }
        ),
    }
)

CONFIG_SCHEMA = cv.All(
    uart.UART_DEVICE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(LD2450),
            cv.Required(UART_ID): cv.use_id(UARTComponent),
            cv.Optional(CONF_NAME, default="LD2450"): cv.string_strict,
            cv.Optional(CONF_TARGETS): cv.All(
                cv.ensure_list(TARGET_SCHEMA),
                cv.Length(min=1, max=3),
            ),
            cv.Optional(CONF_FLIP_X_AXIS, default=False): cv.boolean,
            cv.Optional(CONF_USE_FAST_OFF, default=False): cv.boolean,
            cv.Optional(CONF_OCCUPANCY): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_OCCUPANCY
            ),
            cv.Optional(CONF_MAX_DISTANCE_MARGIN, default="25cm"): cv.All(
                cv.distance, cv.Range(min=0.0, max=6.0)
            ),
            cv.Optional(CONF_MAX_DISTANCE): cv.Any(
                cv.All(cv.distance, cv.Range(min=0.0, max=6.0)),
                number.NUMBER_SCHEMA.extend(
                    {
                        cv.GenerateID(): cv.declare_id(MaxDistanceNumber),
                        cv.Required(CONF_NAME): cv.string_strict,
                        cv.Optional(CONF_INITIAL_VALUE, default="6.0m"): cv.All(
                            cv.distance, cv.Range(min=0.0, max=6.0)
                        ),
                        cv.Optional(CONF_STEP, default="10cm"): cv.All(
                            cv.distance, cv.Range(min=0.0, max=6.0)
                        ),
                        cv.Optional(CONF_RESTORE_VALUE, default=True): cv.boolean,
                        cv.Optional(
                            CONF_UNIT_OF_MEASUREMENT, default=UNIT_METER
                        ): cv.one_of(UNIT_METER, lower="true"),
                    }
                ).extend(cv.COMPONENT_SCHEMA),
            ),
        }
    )
)


def to_code(config):
    """Code generation for the LD2450 component."""
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)

    cg.add(var.set_name(config[CONF_NAME]))
    cg.add(var.set_flip_x_axis(config[CONF_FLIP_X_AXIS]))
    cg.add(var.set_fast_off_detection(config[CONF_USE_FAST_OFF]))
    cg.add(var.set_max_distance_margin(config[CONF_MAX_DISTANCE_MARGIN]))

    # process target list
    if targets_config := config.get(CONF_TARGETS):
        # Register target on controller
        for target_config in targets_config:
            target = yield target_to_code(target_config[CONF_TARGET])
            cg.add(var.register_target(target))

    # Add binary occupancy sensor if present
    if occupancy_config := config.get(CONF_OCCUPANCY):
        occupancy_binary_sensor = yield binary_sensor.new_binary_sensor(
            occupancy_config
        )
        cg.add(var.set_occupancy_binary_sensor(occupancy_binary_sensor))

    # Add max distance value
    if max_distance_config := config.get(CONF_MAX_DISTANCE):
        # Add number component
        if isinstance(max_distance_config, dict):
            max_distance_number = yield number.new_number(
                max_distance_config,
                min_value=0.0,
                max_value=6.0,
                step=max_distance_config[CONF_STEP],
            )
            yield cg.register_parented(max_distance_number, config[CONF_ID])
            yield cg.register_component(max_distance_number, max_distance_config)
            cg.add(
                max_distance_number.set_initial_state(
                    max_distance_config[CONF_INITIAL_VALUE]
                )
            )
            cg.add(
                max_distance_number.set_restore(max_distance_config[CONF_RESTORE_VALUE])
            )
            cg.add(var.set_max_distance_number(max_distance_number))
        elif isinstance(max_distance_config, float):
            # Set fixed value from simple config
            cg.add(var.set_max_distance(max_distance_config))


def target_to_code(config):
    """Code generation for targets within the target list."""
    target = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(target, config)

    if CONF_NAME in config:
        cg.add(target.set_name(config[CONF_NAME]))
    cg.add(target.set_debugging(config[CONF_DEBUG]))
    return target
