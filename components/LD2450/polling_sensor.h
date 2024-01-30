#pragma once
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome::ld2450
{
    /**
     * @brief Simple polling sensor which publishes it's values on a regular basis.
     * Additionally, this sensor converts the output according to the desired unit of measurement.
     */
    class PollingSensor : public sensor::Sensor, public PollingComponent
    {
    public:
        void setup() override
        {
            // Determine unit conversion
            if (unit_of_measurement_ != nullptr)
            {
                if (strcmp(unit_of_measurement_, "m") == 0)
                    conversion_factor_ = 0.001f;
                else if ((strcmp(unit_of_measurement_, "cm") == 0))
                    conversion_factor_ = 0.1f;
            }
        }

        void update() override
        {
            if (raw_state != value_ && !(std::isnan(raw_state) && std::isnan(value_)))
                publish_state(value_);
        }

        /**
         * Gets the unit conversion factor of this sensor.
         * @param new value (distance in mm)
         */
        void set_value(float new_value)
        {
            value_ = new_value * conversion_factor_;
        }

    private:
        /// @brief conversion factor applied to input values
        float conversion_factor_ = 1;

        /// @brief Value of this sensor (un-published)
        float value_ = 0;
    };
} // namespace esphome::ld2450