#include "bluetooth_switch.h"

namespace esphome::ld2450
{
    void BluetoothSwitch::write_state(bool state)
    {
        parent_->set_bluetooth_state(state);
    }
} // namespace esphome::ld2450