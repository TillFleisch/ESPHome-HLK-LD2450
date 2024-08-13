#include "zone.h"

namespace esphome::ld2450
{
    const char *TAG = "Zone";

    bool is_convex(std::vector<Point> polygon)
    {
        if (polygon.size() < 3)
            return false;

        float last_cross_product = NAN;
        int size = polygon.size();
        for (int i = 0; i < size + 1; i++)
        {
            int dx_1 = polygon[(i + 1) % size].x - polygon[i % size].x;
            int dy_1 = polygon[(i + 1) % size].y - polygon[i % size].y;
            int dx_2 = polygon[(i + 2) % size].x - polygon[(i + 1) % size].x;
            int dy_2 = polygon[(i + 2) % size].y - polygon[(i + 1) % size].y;

            // Reject duplicate points
            if ((dx_1 == 0 && dy_1 == 0) || (dx_2 == 0 && dy_2 == 0))
                return false;

            float cross_product = dx_1 * dy_2 - dy_1 * dx_2;
            if (!std::isnan(last_cross_product) && ((cross_product > 0 && last_cross_product < 0) || (cross_product > 0 && last_cross_product < 0)))
                return false;
            last_cross_product = cross_product;
        }
        return true;
    }

    void Zone::dump_config()
    {
        ESP_LOGCONFIG(TAG, "Zone: %s", name_);
        ESP_LOGCONFIG(TAG, "  polygon_size: %i", polygon_.size());
        ESP_LOGCONFIG(TAG, "  polygon valid: %s", is_convex(polygon_) ? "true" : "false");
        if (template_polygon_ != nullptr)
        {
            ESP_LOGCONFIG(TAG, "  template polygon defined");
            ESP_LOGCONFIG(TAG, "  template polygon update interval: %i", int(template_evaluation_interval_));
        }
        ESP_LOGCONFIG(TAG, "  target_timeout: %i", int(target_timeout_));
#ifdef USE_BINARY_SENSOR
        LOG_BINARY_SENSOR("  ", "OccupancyBinarySensor", occupancy_binary_sensor_);
#endif
#ifdef USE_SENSOR
        LOG_SENSOR("  ", "TargetCountSensor", target_count_sensor_);
#endif
    }

    void Zone::update(std::vector<Target *> &targets, bool sensor_available)
    {
        // evaluate custom template polygon at given interval
        if (template_evaluation_interval_ != 0 && millis() - last_template_evaluation_ > template_evaluation_interval_)
        {
            last_template_evaluation_ = millis();
            evaluate_template_polygon();
        }

        if (!sensor_available)
        {
#ifdef USE_BINARY_SENSOR
            if (occupancy_binary_sensor_ != nullptr)
                occupancy_binary_sensor_->publish_state(false);
#endif
#ifdef USE_SENSOR
            if (target_count_sensor_ != nullptr)
                target_count_sensor_->publish_state(NAN);
#endif
            return;
        }

        if (polygon_.size() < 3)
            return;

        int target_count = 0;
        for (Target *target : targets)
        {
            target_count += contains_target(target);
        }

#ifdef USE_BINARY_SENSOR
        if (occupancy_binary_sensor_ != nullptr)
            occupancy_binary_sensor_->publish_state(target_count > 0);
#endif
#ifdef USE_SENSOR
        if (target_count_sensor_ != nullptr && (target_count_sensor_->raw_state != target_count))
            target_count_sensor_->publish_state(target_count);
#endif
    }

    bool Zone::contains_target(Target *target)
    {
        if (polygon_.size() < 3)
            return false;

        // Check if the target is already beeing tracked
        bool is_tracked = tracked_targets_.count(target);
        if (!target->is_present())
        {
            if (!is_tracked)
            {
                return false;
            }
            else
            {
                // Remove from tracking list after timeout (target did not leave via polygon boundary)
                if (millis() - tracked_targets_[target] > target_timeout_)
                {
                    tracked_targets_.erase(target);
                    return false;
                }
                else
                {
                    // Report as contained as long as the target has not timed out
                    return true;
                }
            }
        }

        Point point = Point(target->get_x(), target->get_y());

        int size = polygon_.size();
        bool is_inside = true;
        int16_t min_distance = INT16_MAX;
        float last_cross_product = NAN;
        // Check if the target is inside of the polygon or within the allowed margin, in case it is already tracked
        for (int i = 0; i < size + 1; i++)
        {
            // Check if the target point is on the same side of all edges within the polygon
            int dx_1 = polygon_[(i + 1) % size].x - polygon_[i % size].x;
            int dy_1 = polygon_[(i + 1) % size].y - polygon_[i % size].y;
            int dx_2 = point.x - polygon_[i % size].x;
            int dy_2 = point.y - polygon_[i % size].y;
            float cross_product = dx_1 * dy_2 - dy_1 * dx_2;

            if (!std::isnan(last_cross_product) && ((cross_product > 0 && last_cross_product < 0) || (cross_product > 0 && last_cross_product < 0)))
            {
                is_inside = false;
                // Early stopping for un-tracked targets
                if (!is_tracked)
                    return false;
            }
            last_cross_product = cross_product;

            // Determine the targets distance to the polygon if tracked
            if (is_tracked)
            {
                float dot_product = dx_1 * dx_2 + dy_1 * dy_2;
                float r = dot_product / pow(sqrt(dx_1 * dx_1 + dy_1 * dy_1), 2);

                float distance;
                if (r < 0)
                {
                    distance = sqrt(dx_2 * dx_2 + dy_2 * dy_2);
                }
                else if (r > 1)
                {
                    int dx = polygon_[(i + 1) % size].x - point.x;
                    int dy = polygon_[(i + 1) % size].y - point.y;
                    distance = sqrt(dx * dx + dy * dy);
                }
                else
                {
                    float a = dx_2 * dx_2 + dy_2 * dy_2;
                    float b = pow(sqrt(dx_1 * dx_1 + dy_1 * dy_1) * r, 2);
                    distance = sqrt(a - b);
                }
                min_distance = std::min(min_distance, int16_t(distance));
            }
        }

        if (is_inside && target->is_present())
        {
            // Add and Update last seen time
            tracked_targets_[target] = millis();
        }
        else if (is_tracked && !is_inside)
        {
            // Check if the target is still within the margin of error
            if (min_distance > margin_)
            {
                // Remove from target from tracking list
                if (is_tracked)
                    tracked_targets_.erase(target);
                return false;
            }
        }
        return true;
    }

    bool Zone::evaluate_template_polygon()
    {
        if (template_polygon_ == nullptr)
            return false;

        std::vector<Point> val = (template_polygon_)();
        return update_polygon(val);
    }
} // namespace esphome::ld2450