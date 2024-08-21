#include "LD2450.h"
#include "esphome/core/log.h"

namespace esphome::ld2450
{

    static const char *TAG = "LD2450";

    void LD2450::setup()
    {

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
#ifdef USE_BUTTON
        if (restart_button_ != nullptr)
            restart_button_->add_on_press_callback([this]()
                                                   { this->perform_restart(); });
        if (factory_reset_button_ != nullptr)
            factory_reset_button_->add_on_press_callback([this]()
                                                         { this->perform_factory_reset(); });
#endif
        // Acquire current switch states and update related components
        read_switch_states();
    }

    void LD2450::dump_config()
    {
        ESP_LOGCONFIG(TAG, "LD2450 Hub: %s", name_);
        ESP_LOGCONFIG(TAG, "  fast_off_detection: %s", fast_off_detection_ ? "True" : "False");
        ESP_LOGCONFIG(TAG, "  flip_x_axis: %s", flip_x_axis_ ? "True" : "False");
        ESP_LOGCONFIG(TAG, "  max_detection_tilt_angle: %.2f °", max_detection_tilt_angle_);
        ESP_LOGCONFIG(TAG, "  min_detection_tilt_angle: %.2f °", min_detection_tilt_angle_);
        ESP_LOGCONFIG(TAG, "  max_detection_distance: %i mm", max_detection_distance_);
        ESP_LOGCONFIG(TAG, "  max_distance_margin: %i mm", max_distance_margin_);
        ESP_LOGCONFIG(TAG, "  tilt_angle_margin: %.2f °", tilt_angle_margin_);
#ifdef USE_BINARY_SENSOR
        LOG_BINARY_SENSOR("  ", "OccupancyBinarySensor", occupancy_binary_sensor_);
#endif
#ifdef USE_NUMBER
        LOG_NUMBER("  ", "MaxTiltAngleNumber", max_angle_number_);
        LOG_NUMBER("  ", "MinTiltAngleNumber", min_angle_number_);
        LOG_NUMBER("  ", "MaxDistanceNumber", max_distance_number_);
#endif
#ifdef USE_BUTTON
        LOG_BUTTON("  ", "RestartButton", restart_button_);
        LOG_BUTTON("  ", "FactoryResetButton", factory_reset_button_);
#endif
        LOG_SWITCH("  ", "TrackingModeSwitch", tracking_mode_switch_);
        LOG_SWITCH("  ", "BluetoothSwitch", bluetooth_switch_);
        LOG_SELECT("  ", "BaudRateSelect", baud_rate_select_);
        ESP_LOGCONFIG(TAG, "Zones:");
        if (zones_.size() > 0)
        {
            for (Zone *zone : zones_)
            {
                zone->dump_config();
            }
        }

        // Read and log Firmware-version
        log_sensor_version();
        log_bluetooth_mac();
    }

    const uint8_t update_header[4] = {0xAA, 0xFF, 0x03, 0x00};
    const uint8_t config_header[4] = {0xFD, 0xFC, 0xFB, 0xFA};
    void LD2450::loop()
    {
        // Only process commands if the sensor is not currently restarting / applying changes
        if (!is_applying_changes_ || (is_applying_changes_ && millis() - apply_change_lockout_ > POST_RESTART_LOCKOUT_DELAY))
        {
            is_applying_changes_ = false;

            // Process command queue
            if (command_queue_.size() > 0)
            {
                // Inject enter config mode command if not in mode
                if (!configuration_mode_ && command_queue_.front()[0] != COMMAND_ENTER_CONFIG)
                {
                    command_queue_.insert(command_queue_.begin(), {COMMAND_ENTER_CONFIG, 0x00, 0x01, 0x00});
                }

                // Wait before retransmitting
                if (millis() - command_last_sent_ > COMMAND_RETRY_DELAY)
                {

                    // Remove command form queue after max retries
                    if (command_send_retries_ >= COMMAND_MAX_RETRIES)
                    {
                        if (command_queue_.front()[0] == COMMAND_LEAVE_CONFIG)
                        {
                            // Leave config mode to prevent re-adding the command to the queue (assume config mode already exited)
                            configuration_mode_ = false;
                            command_queue_.erase(command_queue_.begin());
                        }
                        else if (command_queue_.front()[0] == COMMAND_ENTER_CONFIG)
                        {
                            // Clear command queue in case entering config mode failed
                            command_queue_.clear();
                            ESP_LOGW(TAG, "Entering config mode failed, clearing command queue.");
                        }
                        else
                        {
                            command_queue_.erase(command_queue_.begin());
                        }
                        command_send_retries_ = 0;
                        ESP_LOGW(TAG, "Sending command timed out! Is the sensor connected?");
                    }
                    else
                    {
                        std::vector<uint8_t> command = command_queue_.front();
                        write_command(&command[0], command.size());
                        command_last_sent_ = millis();
                        command_send_retries_++;
                    }
                }
            }
            else if (configuration_mode_)
            {
                // Inject leave config command after clearing the queue
                command_queue_.push_back({COMMAND_LEAVE_CONFIG, 0x00});
                command_send_retries_ = 0;
            }
        }

        // Try to process as many messages as possible in a single iteration
        bool processed_message = false;
        do
        {
            processed_message = false;

            // Skip stream until start of message and parse header
            while (!peek_status_ && available() >= 4)
            {
                // Try to read the header and abort on mismatch
                const uint8_t *header;
                uint8_t message_type;
                uint8_t first_byte = read();
                if (first_byte == update_header[0])
                {
                    header = update_header;
                    message_type = 1;
                }
                else if (first_byte == config_header[0])
                {
                    header = config_header;
                    message_type = 2;
                }
                else
                {
                    continue;
                }

                bool header_match = true;
                for (int i = 1; i < 4; i++)
                {
                    if (read() != header[i])
                    {
                        header_match = false;
                        break;
                    }
                }

                if (header_match)
                    // Flag successful header reading
                    peek_status_ = message_type;
            }

            if (peek_status_ == 1 && available() >= 26)
            {
                uint8_t msg[26] = {0x00};
                read_array(msg, 26);
                peek_status_ = 0;

                // Skip invalid messages
                if (msg[24] != 0x55 || msg[25] != 0xCC)
                    return;

                process_message(msg, 24);
                processed_message = true;
            }
            if (peek_status_ == 2 && (available() >= 2 || configuration_message_length_ > 0))
            {
                if (configuration_message_length_ == 0)
                {
                    // Read message content length
                    uint8_t content_length[2];
                    read_array(content_length, 2);
                    configuration_message_length_ = content_length[1] << 8 | content_length[0];
                    // Limit max message length
                    configuration_message_length_ = std::min(configuration_message_length_, 20);
                }

                // Wait until message and frame end are available
                if (available() >= configuration_message_length_ + 4)
                {
                    uint8_t msg[configuration_message_length_ + 4] = {0x00};
                    read_array(msg, configuration_message_length_ + 4);

                    // Assert frame end read correctly
                    if (msg[configuration_message_length_] == 0x04 && msg[configuration_message_length_ + 1] == 0x03 && msg[configuration_message_length_ + 2] == 0x02 && msg[configuration_message_length_ + 3] == 0x01)
                    {
                        process_config_message(msg, configuration_message_length_);
                    }
                    configuration_message_length_ = 0;
                    peek_status_ = 0;
                    processed_message = true;
                }
            }

        } while (processed_message);

        // Detect missing updates from the sensor (not connect or in configuration mode)
        if (sensor_available_ && millis() - last_message_received_ > SENSOR_UNAVAILABLE_TIMEOUT)
        {
            sensor_available_ = false;

            ESP_LOGE(TAG, "LD2450-Sensor stopped sending updates!");

#ifdef USE_BINARY_SENSOR
            if (occupancy_binary_sensor_ != nullptr)
                occupancy_binary_sensor_->publish_state(false);
#endif
#ifdef USE_SENSOR
            if (target_count_sensor_ != nullptr)
                target_count_sensor_->publish_state(NAN);
#endif

            // Update zones and related components (unavailable)
            for (Zone *zone : zones_)
            {
                zone->update(targets_, sensor_available_);
            }

            // Update targets and related components (unavailable)
            for (Target *target : targets_)
            {
                target->clear();
            }
        }

        // Assume the sensor is in it's configuration mode, attempt to leave
        // Attempt to leave config mode periodically if the sensor is not sending updates
        if (!is_applying_changes_ && !sensor_available_ && millis() - last_config_leave_attempt_ > CONFIG_RECOVERY_INTERVAL)
        {
            ESP_LOGD(TAG, "Sensor is not sending updates, attempting to leave config mode.");
            last_config_leave_attempt_ = millis();
            command_send_retries_ = 0;
            configuration_mode_ = true;
            command_queue_.clear();
            command_queue_.push_back({COMMAND_LEAVE_CONFIG, 0x00});
        }

        if (available() != last_available_size_)
        {
            last_available_size_ = available();
            last_available_change_ = millis();
        }

        // Assume the rx buffer has overflowed in the past and is unable to recover - read everything available
        if (available() != 0 && millis() - last_available_change_ > SENSOR_UNAVAILABLE_TIMEOUT / 2)
        {
            // Clear out rx buffer
            ESP_LOGD(TAG, "Clearing RX buffer.");
            while (available())
            {
                read();
            }
        }
    }

    void LD2450::process_message(uint8_t *msg, int len)
    {
        sensor_available_ = true;
        last_message_received_ = millis();
        configuration_mode_ = false;

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

            // Filter targets further than max detection distance and max angle
            float angle = -(atan2(y, x) * (180 / M_PI) - 90);
            if ((y <= max_detection_distance_ || (targets_[i]->is_present() && y <= max_detection_distance_ + max_distance_margin_)) &&
                (angle <= max_detection_tilt_angle_ || (targets_[i]->is_present() && angle <= max_detection_tilt_angle_ + tilt_angle_margin_)) &&
                (angle >= min_detection_tilt_angle_ || (targets_[i]->is_present() && angle >= min_detection_tilt_angle_ - tilt_angle_margin_)))
            {
                targets_[i]->update_values(x, y, speed, distance_resolution);
            }
            else if (y > max_detection_distance_ + max_distance_margin_ ||
                     angle > max_detection_tilt_angle_ + tilt_angle_margin_ ||
                     angle < min_detection_tilt_angle_ - tilt_angle_margin_)
            {
                targets_[i]->clear();
            }
        }

        int target_count = 0;
        for (Target *target : targets_)
        {
            target_count += target->is_present();
        }
        is_occupied_ = target_count > 0;

#ifdef USE_BINARY_SENSOR
        if (occupancy_binary_sensor_ != nullptr)
            occupancy_binary_sensor_->publish_state(is_occupied_);
#endif
#ifdef USE_SENSOR
        if (target_count_sensor_ != nullptr && target_count_sensor_->raw_state != target_count)
            target_count_sensor_->publish_state(target_count);
#endif

        // Update zones and related components
        for (Zone *zone : zones_)
        {
            zone->update(targets_, sensor_available_);
        }
    }

    void LD2450::process_config_message(uint8_t *msg, int len)
    {
        // Remove command from Queue upon receiving acknowledgement
        std::vector<uint8_t> command = command_queue_.front();
        if (command_queue_.size() > 0 && command[0] == msg[0] && msg[1] == 0x01)
        {
            command_queue_.erase(command_queue_.begin());
            command_send_retries_ = 0;
            command_last_sent_ = 0;
        }

        if (msg[0] == COMMAND_ENTER_CONFIG && msg[1] == true)
        {
            configuration_mode_ = true;
        }

        if (msg[0] == COMMAND_LEAVE_CONFIG && msg[1] == true)
        {
            configuration_mode_ = false;
        }

        if ((msg[0] == COMMAND_FACTORY_RESET || msg[0] == COMMAND_RESTART) && msg[1] == true)
        {
            configuration_mode_ = false;

            // Wait for sensor to restart and apply configuration before requesting switch states
            is_applying_changes_ = true;
            apply_change_lockout_ = millis();
        }

        if (msg[0] == COMMAND_READ_VERSION && msg[1] == true)
        {
            ESP_LOGI(TAG, "Sensor Firmware-Version: V%X.%02X.%02X%02X%02X%02X", msg[7], msg[6], msg[11], msg[10], msg[9], msg[8]);
        }

        if (msg[0] == COMMAND_READ_MAC && msg[1] == true)
        {

            bool bt_enabled = !(msg[4] == 0x08 && msg[5] == 0x05 && msg[6] == 0x04 && msg[7] == 0x03 && msg[8] == 0x02 && msg[9] == 0x01);
            if (bluetooth_switch_ != nullptr)
            {
                bluetooth_switch_->publish_state(bt_enabled);
            }

            if (bt_enabled)
            {
                ESP_LOGI(TAG, "Sensor MAC-Address: %02X:%02X:%02X:%02X:%02X:%02X", msg[4], msg[5], msg[6], msg[7], msg[8], msg[9]);
            }
            else
            {
                ESP_LOGI(TAG, "Sensor MAC-Address: Bluetooth disabled!");
            }
        }

        if (msg[0] == COMMAND_READ_TRACKING_MODE && msg[1] == true)
        {
            bool multi_tracking_state = msg[4] == 0x02;
            if (tracking_mode_switch_ != nullptr)
                tracking_mode_switch_->publish_state(multi_tracking_state);
        }
    }

    void LD2450::log_sensor_version()
    {
        const uint8_t read_version[2] = {COMMAND_READ_VERSION, 0x00};
        send_config_message(read_version, 2);
    }

    void LD2450::log_bluetooth_mac()
    {
        const uint8_t read_mac[4] = {COMMAND_READ_MAC, 0x00, 0x01, 0x00};
        send_config_message(read_mac, 4);
    }

    void LD2450::perform_restart()
    {
        const uint8_t restart[2] = {COMMAND_RESTART, 0x00};
        send_config_message(restart, 2);
        read_switch_states();
    }

    void LD2450::perform_factory_reset()
    {
        const uint8_t reset[2] = {COMMAND_FACTORY_RESET, 0x00};
        send_config_message(reset, 2);
        perform_restart();
    }

    void LD2450::set_tracking_mode(bool mode)
    {
        if (mode)
        {
            const uint8_t set_tracking_mode[2] = {COMMAND_MULTI_TRACKING_MODE, 0x00};
            send_config_message(set_tracking_mode, 2);
        }
        else
        {
            const uint8_t set_tracking_mode[2] = {COMMAND_SINGLE_TRACKING_MODE, 0x00};
            send_config_message(set_tracking_mode, 2);
        }

        const uint8_t request_tracking_mode[2] = {COMMAND_READ_TRACKING_MODE, 0x00};
        send_config_message(request_tracking_mode, 2);
    }

    void LD2450::set_bluetooth_state(bool state)
    {
        const uint8_t set_bt[4] = {COMMAND_BLUETOOTH, 0x00, state, 0x00};
        send_config_message(set_bt, 4);
        perform_restart();
    }

    void LD2450::read_switch_states()
    {
        const uint8_t request_tracking_mode[2] = {COMMAND_READ_TRACKING_MODE, 0x00};
        send_config_message(request_tracking_mode, 2);
        log_bluetooth_mac();
    }

    void LD2450::set_baud_rate(BaudRate baud_rate)
    {
        const uint8_t set_baud_rate[4] = {COMMAND_SET_BAUD_RATE, 0x00, baud_rate, 0x00};
        send_config_message(set_baud_rate, 4);
        perform_restart();
    }

    void LD2450::write_command(uint8_t *msg, int len)
    {
        // Write frame header
        write_array({0xFD, 0xFC, 0xFB, 0xFA});

        // Write message length
        write(static_cast<uint8_t>(len));
        write(static_cast<uint8_t>(len >> 8));

        // Write message content
        write_array(msg, len);

        // Write frame end
        write_array({0x04, 0x03, 0x02, 0x01});

        flush();
    }
} // namespace esphome::ld2450
