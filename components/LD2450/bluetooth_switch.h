#pragma once
#include "esphome/components/switch/switch.h"
#include "LD2450.h"

namespace esphome::ld2450
{
    class LD2450;

    /**
     * @brief Switch used for enabling/disabling bluetooth on the LD2450 Sensor
     *
     */
    class BluetoothSwitch : public switch_::Switch, public Parented<LD2450>
    {
    protected:
        void write_state(bool state) override;
    };

} // namespace esphome::ld2450