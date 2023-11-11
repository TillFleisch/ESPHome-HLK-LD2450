#include "LD2450.h"
#include "esphome/core/log.h"

namespace esphome::ld2450
{

    static const char *TAG = "LD2450";

    void LD2450::setup()
    {

        check_uart_settings(256000, 1, uart::UART_CONFIG_PARITY_NONE, 8);

        // Fill target list with mock targets if not present
        for (int i = targets_.size(); i < 3; i++)
        {
            Target *new_target = new Target();
            targets_.push_back(new_target);
        }

        for (int i = 0; i < targets_.size(); i++)
        {
            Target *target = targets_[i];
            // Generate Names if not present
            if (target->get_name() == nullptr)
            {
                std::string name = std::string("Target ").append(std::to_string(i + 1));
                char *cstr = new char[name.length() + 1];
                std::strcpy(cstr, name.c_str());
                target->set_name(cstr);
            }

            target->set_fast_off_detection(fast_off_detection_);
        }

#ifdef USE_BINARY_SENSOR
        if (occupancy_binary_sensor_ != nullptr)
            occupancy_binary_sensor_->publish_initial_state(false);
#endif
    }

    void LD2450::dump_config()
    {
        ESP_LOGCONFIG(TAG, "LD2450 Hub: %s", name_);
#ifdef USE_BINARY_SENSOR
        LOG_BINARY_SENSOR("  ", "OccupancyBinarySensor", occupancy_binary_sensor_);
#endif
    }

    void LD2450::loop()
    {
        // Skip stream until start of message and parse header
        while (!peek_status_ && available() >= 4)
        {
            // Try to read the header and abort on mismatch
            uint8_t header[4] = {0xAA, 0xFF, 0x03, 0x00};
            bool skip = false;
            for (int i = 0; i < 4 && !skip; i++)
            {
                if (read() != header[i])
                    skip = true;
            }
            if (skip)
                continue;

            // Flag successful header reading
            peek_status_ = true;
        }

        if (peek_status_ && available() >= 28)
        {
            uint8_t msg[26] = {0x00};
            read_array(msg, 26);
            peek_status_ = false;

            // Skip invalid messages
            if (msg[24] != 0x55 || msg[25] != 0xCC)
                return;

            process_message(msg, 24);
        }
    }

    void LD2450::process_message(uint8_t *msg, int len)
    {
        for (int i = 0; i < 3; i++)
        {
            int offset = 8 * i;

            int16_t x = msg[offset + 1] << 8 | msg[offset + 0];
            if (msg[offset + 1] & 0x80)
                x = -x + 0x8000;
            int16_t y = (msg[offset + 3] << 8 | msg[offset + 2]);
            if (y != 0)
                y -= 0x8000;
            int speed = msg[offset + 5] << 8 | msg[offset + 4];
            if (msg[offset + 5] & 0x80)
                speed = -speed + 0x8000;
            int distance_resolution = msg[offset + 7] << 8 | msg[offset + 6];

            // Flip x axis if required
            x = x * (flip_x_axis_ ? -1 : 1);

            targets_[i]->update_values(x, y, speed, distance_resolution);
        }

        bool occupied = false;
        for (Target *target : targets_)
        {
            occupied |= target->is_present();
        }
        is_occupied_ = occupied;

#ifdef USE_BINARY_SENSOR
        if (occupancy_binary_sensor_ != nullptr && occupancy_binary_sensor_->state != is_occupied_)
            occupancy_binary_sensor_->publish_state(occupied);
#endif
    }

}