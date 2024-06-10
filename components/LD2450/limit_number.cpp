#include "limit_number.h"

namespace esphome::ld2450
{
    void LimitNumber::setup()
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

    void LimitNumber::control(float value)
    {
        switch (type_)
        {
        case MAX_DISTANCE:
            value = parent_->set_max_distance(value);
            break;
        case MAX_TILT_ANGLE:
            // Use potentially clamped value
            value = parent_->set_max_tilt_angle(value);
            break;
        case MIN_TILT_ANGLE:
            // Use potentially clamped value
            value = parent_->set_min_tilt_angle(value);
            break;
        default:
            break;
        }
        publish_state(value);

        if (this->restore_value_)
            this->pref_.save(&value);
    }
} // namespace esphome::ld2450