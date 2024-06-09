#include "max_tilt_angle_number.h"

namespace esphome::ld2450
{
    void MaxTiltAngleNumber::setup()
    {
        float value;
        if (!restore_value_)
        {
            value = initial_value_;
        }
        else
        {
            // Load value from preference, use initial value if not present
            pref_ = global_preferences->make_preference<float>(get_object_id_hash());
            if (!pref_.load(&value))
                value = initial_value_;
        }
        control(value);
    }

    void MaxTiltAngleNumber::control(float value)
    {
        // Use potentially clamped value
        value = parent_->set_max_tilt_angle(value);
        publish_state(value);

        if (this->restore_value_)
            this->pref_.save(&value);
    }
} // namespace esphome::ld2450