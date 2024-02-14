#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "polling_sensor.h"

#define DEBUG_FREQUENCY 1000
#define FAST_OFF_THRESHOLD 100

namespace esphome::ld2450
{
    /**
     * @brief Target component which provides information about a single target and updates derived sensor components.
     */
    class Target : public Component
    {
    public:
        void setup() override;
        void loop() override;
        void dump_config() override;

        /**
         * @brief Sets the name of this component
         */
        void set_name(const char *name)
        {
            name_ = name;
        }

        /**
         * @brief Sets the debugging flag, which enables/disables raw value debug output.
         */
        void set_debugging(bool flag)
        {
            debug_ = flag;
        }

        /**
         * @brief Sets the fast of detection flag, which determines how the unoccupied state is determined.
         */
        void set_fast_off_detection(bool flag)
        {
            fast_off_detection_ = flag;
        }

        /**
         * @brief Sets the x position sensor reference
         * @param reference polling sensor reference
         */
        void set_x_position_sensor(PollingSensor *x_position_sensor)
        {
            x_position_sensor_ = x_position_sensor;
        }

        /**
         * @brief Sets the y position sensor reference
         * @param reference polling sensor reference
         */
        void set_y_position_sensor(PollingSensor *y_position_sensor)
        {
            y_position_sensor_ = y_position_sensor;
        }

        /**
         * @brief Sets the speed sensor reference
         * @param reference polling sensor reference
         */
        void set_speed_sensor(PollingSensor *speed_sensor)
        {
            speed_sensor_ = speed_sensor;
        }

        /**
         * @brief Sets the distance resolution sensor reference
         * @param reference polling sensor reference
         */
        void set_distance_resolution_sensor(PollingSensor *distance_resolution_sensor)
        {
            distance_resolution_sensor_ = distance_resolution_sensor;
        }

        /**
         * @brief Sets the angle sensor reference
         * @param reference polling sensor reference
         */
        void set_angle_sensor(PollingSensor *angle_sensor)
        {
            angle_sensor_ = angle_sensor;
        }

        /**
         * @brief Sets the distance sensor reference
         * @param reference polling sensor reference
         */
        void set_distance_sensor(PollingSensor *distance_sensor)
        {
            distance_sensor_ = distance_sensor;
        }

        /**
         * @brief Updates the value in this target object
         * @param x The x coordinate of the target
         * @param y The y coordinate of the target
         * @param speed The speed of the target
         * @param resolution The distance resolution of the measurement
         */
        void update_values(int16_t x, int16_t y, int16_t speed, int16_t resolution);

        /**
         * @brief Determines whether this target is currently detected.
         * @return true if the target is detected, false otherwise
         */
        bool is_present();

        /**
         * @brief Determines if this target is currently moving
         * @return true if the target is moving, false if it is stationary.
         */
        bool is_moving()
        {
            return speed_ != 0;
        }

        /**
         * @brief Time since last last change in values.
         * @return timestamp in milliseconds since start
         */
        uint32_t get_last_change()
        {
            return last_change_;
        }

        /**
         * @brief Rests all values in this target
         */
        void clear()
        {
            update_values(0, 0, 0, 0);
        }

        /**
         * @brief Gets the name of this target
         */
        const char *get_name()
        {
            return name_;
        }

        /**
         * Gets the x coordinate (horizontal position) of this targets
         *
         * @return horizontal position of the target (0 = center)
         */
        int16_t get_x()
        {
            return x_;
        }

        /**
         * Gets the y coordinate (distance from the sensor) of this target
         * @return distance in centimeters
         */
        int16_t get_y()
        {
            return y_;
        }

        /**
         * Gets the movement speed of this target
         * @return speed in m/s
         */
        int16_t get_speed()
        {
            return speed_;
        }

        /**
         * Gets the distance resolution of this target
         * @return distance error
         */
        int16_t get_distance_resolution()
        {
            return resolution_;
        }

    protected:
        /// @brief X (horizontal) coordinate of the target in relation to the sensor
        int16_t x_ = 0;

        /// @brief Y (distance) coordinate of the target in relation to the sensor
        int16_t y_ = 0;

        /// @brief speed of the target
        int16_t speed_ = 0;

        /// @brief distance resolution of the target
        int16_t resolution_ = 0;

        /// @brief  Name of this target
        const char *name_ = nullptr;

        /// @brief Debugging flag, enables logs for value changes
        bool debug_ = false;

        /// @brief Determines whether the fast unoccupied detection method is applied
        bool fast_off_detection_ = false;

        /// @brief time stamp of the last debug message which was sent.
        uint32_t last_debug_message_ = 0;

        /// @brief time of the last value change
        uint32_t last_change_ = 0;

        /// @brief sensor reference of the x position sensor
        PollingSensor *x_position_sensor_ = nullptr;

        /// @brief sensor reference of the y position sensor
        PollingSensor *y_position_sensor_ = nullptr;

        /// @brief sensor reference of the speed sensor
        PollingSensor *speed_sensor_ = nullptr;

        /// @brief sensor reference of the distance resolution sensor
        PollingSensor *distance_resolution_sensor_ = nullptr;

        /// @brief sensor reference of the angle sensor
        PollingSensor *angle_sensor_ = nullptr;

        /// @brief sensor reference of the distance sensor
        PollingSensor *distance_sensor_ = nullptr;
    };
} // namespace esphome::ld2450