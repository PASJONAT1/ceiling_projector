cpp
#pragma once

#include "esphome.h"

class CeilingProjector : public Component {
  public:
    CeilingProjector(GPIOPin *cs_pin, GPIOPin *clk_pin, GPIOPin *data_pin);
    void setup() override;
    void loop() override;
};
