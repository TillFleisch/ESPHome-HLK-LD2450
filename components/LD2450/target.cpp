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
    }

    void Target::loop()
    {
        if (debug_)
        {
            if (millis() - last_debug_message_ > DEBUG_FREQUENCY)
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
    }

    bool Target::is_present()
    {
        return resolution_ != 0 && (!fast_off_detection_ || millis() - last_change_ <= FAST_OFF_THRESHOLD);
    }

}