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
        Point()
            : x(0), y(0)
        {
        }
        Point(int x, int y)
            : x(x), y(y)
        {
        }
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
         * @param available True if the sensor is currently available, false otherwise
         * */
        void update(std::vector<Target *> &targets, bool sensor_available);

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
            return true;
        }

        /**
         * @brief Defines a template polygon which will be evaluated regularly
         */
        void set_template_polygon(std::function<std::vector<Point>()> &&template_polygon)
        {
            this->template_polygon_ = template_polygon;
        }

        /**
         * @brief Sets the interval at which template polygons are evaluated
         */
        void set_template_evaluation_interval(uint32_t interval)
        {
            template_evaluation_interval_ = interval;
        }

        /**
         * @brief Evaluates the template polygon which is configured for this zone
         * @return false if the polygon is not defined or invalid, true otherwise
         */
        bool evaluate_template_polygon();

        /**
         * @brief Retrieves the currently used polygon
         * @return list of points which make up the current polygon
         */
        std::vector<Point> get_polygon()
        {
            return polygon_;
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

        /// @brief Margin around the polygon in mm, in which existing targets are tracked further
        uint16_t margin_ = 250;

        /// @brief timeout after which a target within the is considered absent
        int target_timeout_ = 5000;

        /// @brief Map of targets which are currently tracked inside of this polygon with their last seen timestamp
        std::map<Target *, uint32_t> tracked_targets_{};

        /// @brief Template polygon function
        std::function<std::vector<Point>()> template_polygon_ = nullptr;

        /// @brief timestamp of the last template evaluation
        uint32_t last_template_evaluation_ = 0;

        /// @brief interval in which the polygon template is evaluated (time in ms); 0 for no updates
        uint32_t template_evaluation_interval_ = 1000;
    };

    template <typename... Ts>
    class UpdatePolygonAction : public Action<Ts...>
    {
    public:
        UpdatePolygonAction(Zone *parent)
            : parent_(parent)
        {
        }

        TEMPLATABLE_VALUE(std::vector<Point>, polygon)

        void play(const Ts &...x) override
        {
            std::vector<Point> polygon = this->polygon_.value(x...);
            this->parent_->update_polygon(polygon);
        }

        Zone *parent_;
    };
} // namespace esphome::ld2450
