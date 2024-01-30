#pragma once
#include "esphome/components/select/select.h"
#include "LD2450.h"

namespace esphome::ld2450
{
    class LD2450;

    /**
     * @brief Select option used for setting the sensors baud rate
     *
     */
    class BaudRateSelect : public select::Select, public Parented<LD2450>
    {
    protected:
        void control(const std::string &value);
    };

} // namespace esphome::ld2450