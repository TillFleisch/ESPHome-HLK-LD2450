#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/helpers.h"
#include "target.h"
#include "zone.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif

#define COMMAND_MAX_RETRIES 5
#define COMMAND_RETRY_DELAY 100

#define COMMAND_ENTER_CONFIG 0xFF
#define COMMAND_LEAVE_CONFIG 0xFE
#define COMMAND_READ_VERSION 0xA0

namespace esphome::ld2450
{
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
         * @brief Sets the maximum detection distance
         * @param distance maximum distance in meters
         */
        void set_max_distance(float distance)
        {
            if (!std ::isnan(distance))
                max_detection_distance_ = int(distance * 1000);
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
         * @brief Reads and logs the sensors version number.
         */
        void log_sensor_version()
        {
            uint8_t read_version[2] = {COMMAND_READ_VERSION, 0x00};
            send_config_message(read_version, 2);
        }

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
        void send_config_message(uint8_t *msg, int len)
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

        /// @brief Expected length of the configuration message
        int configuration_message_length_ = 0;

        /// @brief timestamp of the last message which was sent to the sensor
        long command_last_sent_ = 0;

        /// @brief Queue of commands to execute
        std::vector<std::vector<uint8_t>> command_queue_;

        /// @brief Nr of times the command has been written to UART
        int command_send_retries_ = 0;

        /// @brief The maximum detection distance in mm
        int16_t max_detection_distance_ = 6000;

        /// @brief The margin added to the max detection distance in which a detect target still counts as present, even though it is outside of the max detection distance
        int16_t max_distance_margin_ = 250;

        /// @brief List of registered and mock tracking targets
        std::vector<Target *> targets_;

        /// @brief List of registered zones
        std::vector<Zone *> zones_;
    };
}