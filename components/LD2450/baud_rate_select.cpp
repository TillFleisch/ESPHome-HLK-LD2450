#include "baud_rate_select.h"

namespace esphome::ld2450
{
    void BaudRateSelect::control(const std::string &value)
    {
        parent_->set_baud_rate(BAUD_STRING_TO_ENUM.at(value));
    }
} // namespace esphome::ld2450