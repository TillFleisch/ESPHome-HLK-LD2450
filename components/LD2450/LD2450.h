#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/helpers.h"
#include "target.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

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

    protected:
        /**
         * @brief Parses the input message and updates related components.
         * @param msg Message buffer
         * @param len Message content
         */
        void process_message(uint8_t *msg, int len);

        /// @brief indicated whether the start sequence has been parsed
        bool peek_status_ = false;

        /// @brief Name of this component
        const char *name_ = "LD2450";

        /// @brief Determines whether the x values are inverted
        bool flip_x_axis_ = false;

        /// @brief indicates whether a target is detected
        bool is_occupied_ = false;

        /// @brief Determines whether the fast unoccupied detection method is applied
        bool fast_off_detection_ = false;

        /// @brief List of registered and mock tracking targets
        std::vector<Target *> targets_;
    };
}