#pragma once
#include <map>
#include "target.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
namespace esphome::ld2450
{
    /**
     * @brief Simple point class which describes Cartesian location.
     */
    class Point
    {
    public:
        Point() : x(0), y(0) {}
        Point(int x, int y) : x(x), y(y) {}
        int x, y;
    };

    /**
     * @brief Checks if the provided polygon is convex.
     * @return true if the polygon is convex, false otherwise.
     */
    bool is_convex(std::vector<Point> polygon);

    /**
     * @brief Zones describe a phyiscal area, in which target are tracked. The area is given by a convex polygon.
     */
    class Zone
    {
#ifdef USE_BINARY_SENSOR
        SUB_BINARY_SENSOR(occupancy)
#endif
#ifdef USE_SENSOR
        SUB_SENSOR(target_count)
#endif
    public:
        /**
         * @brief Update the name of this zone.
         * @param name new name
         */
        void set_name(const char *name)
        {
            name_ = name;
        }

        /**
         * @brief Sets the margin used on this zone.
         * @param margin margin in m
         */
        void set_margin(float margin)
        {
            margin_ = int(margin * 1000);
        }

        /**
         * @brief Sets the target timeout which is used for targets inside of the polygon.
         * @param time time in ms
         */
        void set_target_timeout(int time)
        {
            target_timeout_ = time;
        }

        /**
         * @brief Updates sensors related to this zone.
         * @param targets Reference to a vector of targets which will be used for calculation
         * */
        void update(std::vector<Target *> &targets);

        /**
         * Logs the Zone configuration.
         */
        void dump_config();

        /**
         * Gets the occupancy status of this Zone.
         * @return true, if at least one target is present in this zone.
         */
        bool is_occupied()
        {
            return tracked_targets_.size() > 0;
        }

        /**
         * @brief Gets the number of targets currently occupying this zone.
         * @return number of targets
         */
        uint8_t get_target_count()
        {
            return tracked_targets_.size();
        }

        /**
         * @brief Adds a new point to the polygon of this zone.
         * @param x coordinate in m
         * @param y coordinate in m
         */
        void append_point(float x, float y)
        {
            polygon_.push_back(Point(int(x * 1000), int(y * 1000)));
        };

        /**
         * @brief Updates the polygon of this zone.
         * @param polygon new convex polygon
         * @return true if the new polygon is convex, false otherwise
         */
        bool update_polygon(std::vector<Point> polygon)
        {
            if (!is_convex(polygon))
                return false;
            polygon_ = polygon;
        }

    protected:
        /**
         * @brief checks if a Target is contained within the zone
         * @return true if the target is currently tracked inside this zone.
         */
        bool contains_target(Target *target);

        /// @brief Name of this zone
        const char *name_ = "Unnamed Zone";

        /// @brief List of points which make up a convex polygon
        std::vector<Point> polygon_{};

        /// @brief Margin around the polygon, which still in mm
        uint16_t margin_ = 250;

        /// @brief timeout after which a target within the is considered absent
        int target_timeout_ = 5000;

        /// @brief List of targets which are currently tracked inside of this polygon
        std::vector<Target *> tracked_targets_{};
    };
}
