#pragma once
#include "esphome/components/number/number.h"
#include "esphome/core/preferences.h"
#include "esphome/core/component.h"
#include "LD2450.h"

namespace esphome::ld2450
{

    /**
     * User controlled number component which updates the max detection distance on it's parent.
     */
    class MaxDistanceNumber : public number::Number, public Component, public Parented<LD2450>
    {
    public:
        void setup() override;

        /**
         * @brief Sets the initial value of this number.
         * @param value new value
         */
        void set_initial_state(float value)
        {
            initial_value_ = value;
        }

        /**
         * @brief Sets the restore flag. If set to true, this component will attempt to restore the value instead of using the initial value.
         * @param restore_value new value
         */
        void set_restore(bool restore_value)
        {
            restore_value_ = restore_value;
        }

    protected:
        /**
         * @brief Action performed when a new value is available. Forwards the value to the parent component. Updates preferences if required.
         * @param value new maximum distance value
         */
        void control(float value) override;

        /// @brief Initial value of this number component
        float initial_value_ = 6.0f;

        /// @brief Value restore flag. If set to true, the initial value will be restored from memory.
        bool restore_value_ = true;

        /// @brief Preference management reference
        ESPPreferenceObject pref_;
    };

} // namespace esphome::ld2450