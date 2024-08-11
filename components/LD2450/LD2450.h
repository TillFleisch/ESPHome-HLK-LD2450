#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/helpers.h"
#include "target.h"
#include "zone.h"
#include "tracking_mode_switch.h"
#include "bluetooth_switch.h"
#include "baud_rate_select.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_BUTTON
#include "esphome/components/button/button.h"
#endif

#define SENSOR_UNAVAILABLE_TIMEOUT 4000
#define CONFIG_RECOVERY_INTERVAL 60000
#define POST_RESTART_LOCKOUT_DELAY 2000

#define COMMAND_MAX_RETRIES 10
#define COMMAND_RETRY_DELAY 100

#define COMMAND_ENTER_CONFIG 0xFF
#define COMMAND_LEAVE_CONFIG 0xFE
#define COMMAND_READ_VERSION 0xA0
#define COMMAND_RESTART 0xA3
#define COMMAND_FACTORY_RESET 0xA2

#define COMMAND_READ_TRACKING_MODE 0x91
#define COMMAND_SINGLE_TRACKING_MODE 0x80
#define COMMAND_MULTI_TRACKING_MODE 0x90

#define COMMAND_READ_MAC 0xA5
#define COMMAND_BLUETOOTH 0xA4

#define COMMAND_SET_BAUD_RATE 0xA1

namespace esphome::ld2450
{

    enum BaudRate
    {
        BAUD_9600 = 0x01,
        BAUD_19200 = 0x02,
        BAUD_38400 = 0x03,
        BAUD_57600 = 0x04,
        BAUD_115200 = 0x05,
        BAUD_230400 = 0x06,
        BAUD_256000 = 0x07,
        BAUD_460800 = 0x08,
    };

    static const std::map<std::string, BaudRate> BAUD_STRING_TO_ENUM{
        {"9600", BAUD_9600},
        {"19200", BAUD_19200},
        {"38400", BAUD_38400},
        {"57600", BAUD_57600},
        {"115200", BAUD_115200},
        {"230400", BAUD_230400},
        {"256000", BAUD_256000},
        {"460800", BAUD_460800}};

    class TrackingModeSwitch;
    class BluetoothSwitch;
    class BaudRateSelect;

    /**
     * @brief Empty button definition used as a template for the restart and factory reset buttons.
     */
    class EmptyButton : public button::Button
    {
    protected:
        virtual void press_action(){};
    };

    /**
     * @brief UART component responsible for processing the data stream provided by the HLK-LD2450 sensor
     */
    class LD2450 : public uart::UARTDevice, public Component
    {
#ifdef USE_BINARY_SENSOR
        SUB_BINARY_SENSOR(occupancy)
#endif
#ifdef USE_SENSOR
        SUB_SENSOR(target_count)
#endif
#ifdef USE_NUMBER
        SUB_NUMBER(max_distance)
        SUB_NUMBER(max_angle)
        SUB_NUMBER(min_angle)
#endif
#ifdef USE_BUTTON
        SUB_BUTTON(restart)
        SUB_BUTTON(factory_reset)
#endif
    public:
        void setup() override;
        void loop() override;
        void dump_config() override;

        /**
         * @brief Sets the name of this component
         * @param name new name of the component
         */
        void set_name(const char *name)
        {
            name_ = name;
        }

        /**
         * @brief Adds a target component to the list of targets.
         * @param target Target to add
         */
        void register_target(Target *target)
        {
            targets_.push_back(target);
        }

        /**
         * @brief Adds a zone to the list of registered zones.
         */
        void register_zone(Zone *zone)
        {
            zones_.push_back(zone);
        }

        /**
         * @brief Sets the x axis inversion flag
         * @param flip true if the x axis should be flipped, false otherwise
         */
        void set_flip_x_axis(bool flip)
        {
            flip_x_axis_ = flip;
        }

        /**
         * @brief Sets the fast of detection flag, which determines how the unoccupied state is determined.
         * @param value true if the x axis flipped
         */
        void set_fast_off_detection(bool value)
        {
            fast_off_detection_ = value;
        }

        /**
         * @brief Sets the maximum tilt angle which is detected (clamped between min tilt angle and 90)
         * @param angle maximum detected angle in degrees
         * @return the new (clamped) max tilt value
         */
        float set_max_tilt_angle(float angle)
        {
            if (!std::isnan(angle))
                max_detection_tilt_angle_ = std::max(std::min(angle, 90.0f), min_detection_tilt_angle_ + 1.0f);
            return max_detection_tilt_angle_;
        }

        /**
         * @brief Sets the minimum tilt angle which is detected (clamped between max tilt angle and -90)
         * @param angle minimum detected angle in degrees
         * @return the new (clamped) min tilt value
         */
        float set_min_tilt_angle(float angle)
        {
            if (!std ::isnan(angle))
                min_detection_tilt_angle_ = std::min(std::max(angle, -90.0f), max_detection_tilt_angle_ - 1.0f);
            return min_detection_tilt_angle_;
        }

        /**
         * @brief Sets the margin which used for angle limitations
         * This margin is added to the min/max tilt angle, such that detected targets still counts as present, even though they are outside of the min/max detection angle. This can be used to reduce flickering.
         * @param angle angle in degrees
         */
        void set_tilt_angle_margin(float angle)
        {
            if (!std ::isnan(angle))
                tilt_angle_margin_ = angle;
        }

        /**
         * @brief Sets the maximum detection distance
         * @param distance maximum distance in meters
         * @return the new maximum distance
         */
        float set_max_distance(float distance)
        {
            if (!std ::isnan(distance))
                max_detection_distance_ = int(distance * 1000);
            return distance;
        }

        /**
         * @brief Sets the maximum distance detection margin.
         * This margin is added to the max detection distance, such that detected targets still counts as present, even though they are outside of the max detection distance. This can be used to reduce flickering.
         * @param distance margin distance in m
         */
        void set_max_distance_margin(float distance)
        {
            if (!std ::isnan(distance))
                max_distance_margin_ = int(distance * 1000);
        }

        /**
         * @brief Set the tracking mode switch for this sensor
         *
         * @param switch_ switch reference
         */
        void set_tracking_mode_switch(TrackingModeSwitch *switch_)
        {
            tracking_mode_switch_ = switch_;
        }

        /**
         * @brief Set the bluetooth switch for this sensor
         *
         * @param switch_ switch reference
         */
        void set_bluetooth_switch(BluetoothSwitch *switch_)
        {
            bluetooth_switch_ = switch_;
        }

        /**
         * @brief Set the baud rate selector reference on this sensor
         *
         * @param select select component reference
         */
        void set_baud_rate_select(BaudRateSelect *select)
        {
            baud_rate_select_ = select;
        }

        /**
         * @brief Gets the occupancy status of this LD2450 sensor.
         * @return true if at least one target is present, false otherwise
         */
        bool is_occupied()
        {
            return is_occupied_;
        }

        /**
         * @brief Gets the specified target from this device.
         * @param i target index
         */
        Target *get_target(int i)
        {
            if (i < 0 && i >= targets_.size())
                return nullptr;
            return targets_[i];
        }

        /**
         * @brief Gets the state of the connected sensor
         * @return True if the sensor is connected and communicating, False otherwise
         */
        bool is_sensor_available()
        {
            return sensor_available_;
        }

        /**
         * @brief Reads and logs the sensors version number.
         */
        void log_sensor_version();

        /**
         * @brief Reads and logs the sensors mac address.
         */
        void log_bluetooth_mac();

        /**
         * @brief Restarts the sensor module
         */
        void perform_restart();

        /**
         * @brief Resets the module to it's factory default settings and performs a restart.
         */
        void perform_factory_reset();

        /**
         * @brief Set the sensors target tracking mode
         *
         * @param mode true for multi target mode, false for single target tracking mode
         */
        void set_tracking_mode(bool mode);

        /**
         * @brief Set the bluetooth state on the sensor
         *
         * @param state true if bluetooth should be enabled, false otherwise
         */
        void set_bluetooth_state(bool state);

        /**
         * @brief Requests the state of switches from the sensor.
         *
         */
        void read_switch_states();

        /**
         * @brief Set the sensors baud rate
         *
         * @param baud_rate New Baud Rate
         */
        void set_baud_rate(BaudRate baud_rate);

    protected:
        /**
         * @brief Parses the input message and updates related components.
         * @param msg Message buffer
         * @param len Message content
         */
        void process_message(uint8_t *msg, int len);

        /**
         * @brief Parses the input configuration-message and updates related components.
         * @param msg Message buffer
         * @param len Message length
         */
        void process_config_message(uint8_t *msg, int len);

        /**
         * @brief Generates message header/end and writes the command to UART
         * @param msg command buffer
         * @param len command length
         */
        void write_command(uint8_t *msg, int len);

        /**
         * @brief Submits a config message for being sent out. If the config command is not acknowledged after a fixed retry count, the command will be discarded.
         * @param msg Message buffer
         * @param msg Message length
         */
        void send_config_message(const uint8_t *msg, int len)
        {
            command_queue_.push_back(std::vector<uint8_t>(msg, msg + len));
        }

        /// @brief indicates whether the start sequence has been parsed
        uint8_t peek_status_ = 0;

        /// @brief Name of this component
        const char *name_ = "LD2450";

        /// @brief Determines whether the x values are inverted
        bool flip_x_axis_ = false;

        /// @brief indicates whether a target is detected
        bool is_occupied_ = false;

        /// @brief Determines whether the fast unoccupied detection method is applied
        bool fast_off_detection_ = false;

        /// @brief Determines whether the sensor is in it's configuration mode
        bool configuration_mode_ = false;

        /// @brief Indicated that the sensor is currently factory resetting
        bool is_applying_changes_ = false;

        /// @brief indicates if the sensor is communicating
        bool sensor_available_ = false;

        /// @brief Expected length of the configuration message
        int configuration_message_length_ = 0;

        /// @brief timestamp of the last message which was sent to the sensor
        uint32_t command_last_sent_ = 0;

        /// @brief timestamp of the last received message
        uint32_t last_message_received_ = 0;

        /// @brief timestamp at which the last available size change has occurred. Once the rx buffer has overflown it must be cleared manually on some configurations to receive new data
        uint32_t last_available_change_ = 0;

        /// @brief timestamp of the last attempt to leave config mode if it's not responding
        uint32_t last_config_leave_attempt_ = 0;

        /// @brief timestamp of lockout period after applying changes requiring a restart
        uint32_t apply_change_lockout_ = 0;

        /// @brief nr of available bytes during the last iteration
        int last_available_size_ = 0;

        /// @brief Queue of commands to execute
        std::vector<std::vector<uint8_t>> command_queue_;

        /// @brief Nr of times the command has been written to UART
        int command_send_retries_ = 0;

        /// @brief The maximum detection angle in degrees
        float max_detection_tilt_angle_ = 90;

        /// @brief The minimum detection angle in degrees
        float min_detection_tilt_angle_ = -90;

        /// @brief The margin added to tilt angle detection limitations
        float tilt_angle_margin_ = 5;

        /// @brief The maximum detection distance in mm
        int16_t max_detection_distance_ = 6000;

        /// @brief The margin added to the max detection distance in which a detect target still counts as present, even though it is outside of the max detection distance
        int16_t max_distance_margin_ = 250;

        /// @brief List of registered and mock tracking targets
        std::vector<Target *> targets_;

        /// @brief List of registered zones
        std::vector<Zone *> zones_;

        /// @brief Tracking mode switch which enables/disables multi-target tracking
        TrackingModeSwitch *tracking_mode_switch_ = nullptr;

        /// @brief Sensor Bluetooth switch which enables/disables bluetooth on the sensor
        BluetoothSwitch *bluetooth_switch_ = nullptr;

        /// @brief Select options used for setting the sensors baud rate
        BaudRateSelect *baud_rate_select_ = nullptr;
    };
} // namespace esphome::ld2450