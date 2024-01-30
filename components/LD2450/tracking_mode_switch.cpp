#include "tracking_mode_switch.h"

namespace esphome::ld2450
{
    void TrackingModeSwitch::write_state(bool state)
    {
        parent_->set_tracking_mode(state);
    }
} // namespace esphome::ld2450