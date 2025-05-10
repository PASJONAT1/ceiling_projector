cpp
#pragma once

#include "esphome.h"

class CeilingProjector : public Component {
        public:
          CeilingProjector(GPIOPin *cs_pin, GPIOPin *clk_pin, GPIOPin *data_pin)
              : cs_pin_(cs_pin), clk_pin_(clk_pin), data_pin_(data_pin) {
            hours_ = 12;
            minutes_ = 34;
            brightness_ = 0;
            colon_enabled_ = true;
            display_mode_ = 0; // 0 = normalny czas, 1 = czas zmywarki
    void setup() override;
    void loop() override;
};
