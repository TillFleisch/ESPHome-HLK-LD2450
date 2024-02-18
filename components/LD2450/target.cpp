#include "esphome/core/log.h"
#include "target.h"

namespace esphome::ld2450
{
    static const char *TAG = "Target";

    void Target::setup()
    {
    }

    void Target::dump_config()
    {
        std::string name = name_ != nullptr ? name_ : "Unnamed Target";
        ESP_LOGCONFIG(TAG, "Target: %s", name.c_str());
        ESP_LOGCONFIG(TAG, "  debug: %s", debug_ ? "True" : "False");
        LOG_SENSOR("  ", "X Position", x_position_sensor_);
        LOG_SENSOR("  ", "Y Position", y_position_sensor_);
        LOG_SENSOR("  ", "Speed", speed_sensor_);
        LOG_SENSOR("  ", "Distance Resolution", distance_resolution_sensor_);
        LOG_SENSOR("  ", "Angle", angle_sensor_);
        LOG_SENSOR("  ", "Distance", distance_sensor_);
    }

    void Target::loop()
    {
        if (debug_)
        {
            // Only show debug messages if updates are available
            if (is_present() && millis() - last_debug_message_ > DEBUG_FREQUENCY)
            {
                last_debug_message_ = millis();
                std::string name = name_ != nullptr ? name_ : "Unnamed Target";
                ESP_LOGD(TAG, "Target %s: x:%i; y:%i; speed:%i; res:%i;", name.c_str(), x_, y_, speed_, resolution_);
            }
        }
    }

    void Target::update_values(int16_t x, int16_t y, int16_t speed, int16_t resolution)
    {
        if (fast_off_detection_ && resolution_ != 0 &&
            (x != x_ || y != y_ || speed != speed_ || resolution != resolution_))
            last_change_ = millis();
        x_ = x;
        y_ = y;
        speed_ = speed;
        resolution_ = resolution;

        bool present = is_present();
        // Update sub sensors
        if (x_position_sensor_ != nullptr)
            x_position_sensor_->set_value(present ? x_ : NAN);
        if (y_position_sensor_ != nullptr)
            y_position_sensor_->set_value(present ? y_ : NAN);
        if (speed_sensor_ != nullptr)
            speed_sensor_->set_value(present ? speed_ : NAN);
        if (distance_resolution_sensor_ != nullptr)
            distance_resolution_sensor_->set_value(present ? resolution_ : NAN);
        if (angle_sensor_ != nullptr)
        {
            float angle = atan2(y, x) * (180 / M_PI) - 90;
            angle_sensor_->set_value(present ? -angle : NAN);
        }
        if (distance_sensor_ != nullptr)
        {
            float distance = sqrt(x_ * x_ + y_ * y_);
            distance_sensor_->set_value(present ? distance : NAN);
        }
    }

    bool Target::is_present()
    {
        return resolution_ != 0 && (!fast_off_detection_ || millis() - last_change_ <= FAST_OFF_THRESHOLD);
    }

} // namespace esphome::ld2450