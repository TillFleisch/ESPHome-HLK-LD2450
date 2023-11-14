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
#ifdef USE_BINARY_SENSOR
        LOG_BINARY_SENSOR("  ", "OccupancyBinarySensor", occupancy_binary_sensor_);
#endif
#ifdef USE_SENSOR
        LOG_SENSOR("  ", "TargetCountSensor", target_count_sensor_);
#endif
    }

    void Zone::update(std::vector<Target *> &targets)
    {
        if (polygon_.size() < 3)
            return;

        int target_count = 0;
        for (Target *target : targets)
        {
            target_count += contains_target(target);
        }

#ifdef USE_BINARY_SENSOR
        if (occupancy_binary_sensor_ != nullptr && (occupancy_binary_sensor_->state != (target_count > 0) || !occupancy_binary_sensor_->has_state()))
            occupancy_binary_sensor_->publish_state(target_count > 0);
#endif
#ifdef USE_SENSOR
        if (target_count_sensor_ != nullptr && (target_count_sensor_->state != target_count || !target_count_sensor_->has_state()))
            target_count_sensor_->publish_state(target_count);
#endif
    }

    bool Zone::contains_target(Target *target)
    {
        if (polygon_.size() < 3)
            return false;

        // Check if the target is already beeing tracked
        auto it = std::find(tracked_targets_.begin(), tracked_targets_.end(), target);
        bool is_tracked = (it != tracked_targets_.end());

        if (!target->is_present())
        {
            // Remove from tracking list
            if (it != tracked_targets_.end())
                tracked_targets_.erase(it);

            return false;
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

        if (!is_tracked && is_inside)
        {
            // Add newly tracked targets
            tracked_targets_.push_back(target);
        }
        else if (is_tracked && !is_inside)
        {
            // Check if the target is still within the margin of error
            if (min_distance > margin_)
            {
                // Remove from target from tracking list
                if (it != tracked_targets_.end())
                    tracked_targets_.erase(it);
                return false;
            }
        }
        return true;
    }
}