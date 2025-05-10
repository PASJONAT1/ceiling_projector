cpp
#pragma once

#include "esphome.h"

  - lambda: |-
      
            
            // Inicjalizacja statusu
            status_ = "Initializing";
          }
      
          void setup() override {
            // Konfiguracja pinów
            this->cs_pin_->setup();
            this->clk_pin_->setup();
            this->data_pin_->setup();
            
            this->cs_pin_->digital_write(true);
            this->clk_pin_->digital_write(true);
            this->data_pin_->digital_write(true);
            
            // Inicjalizacja wyświetlacza
            status_ = "Initializing display...";
            delay(500);
            lcdShift(0b100111000110);  // normal
            lcdShift(0b100000000010);  // sysEn
            lcdShift(0b100001010010);  // bias13x4
            lcdShift(0b100000110000);  // rc256k
            lcdShift(0b100000000110);  // lcdOn
            
            // Wyczyść wyświetlacz
            clearAll();
            
            // Wyświetl początkowy czas
            displayTime();
            
            status_ = "Ready";
          }
      
          void loop() override {
            // Sprawdzanie, czy zmywarka jest aktywna - to możemy robić w loop
            if (id(dishwasher_2021).has_state() && id(dishwasher_2021).state == "ON") {
              // Zmywarka jest aktywna - sprawdź czy mamy aktualizować tryb
              if (display_mode_ == 0 && id(display_dishwasher_switch).state) {
                set_mode(1);
              }
            } else {
              // Zmywarka jest nieaktywna - sprawdź czy mamy aktualizować tryb
              if (display_mode_ == 1 && !id(display_dishwasher_switch).state) {
                set_mode(0);
              }
            }
            
            // Aktualizuj status co 30 sekund
            static uint32_t last_update = 0;
            if (millis() - last_update > 30000) {
              last_update = millis();
              update_status();
            }
          }
          
          void set_time(int hours, int minutes) {
            hours_ = hours;
            minutes_ = minutes;
            if (display_mode_ == 0) {
              displayTime();
            }
          }
          
          void set_dishwasher_time(int hours, int minutes) {
            dishwasher_hours_ = hours;
            dishwasher_minutes_ = minutes;
            if (display_mode_ == 1) {
              displayDishwasherTime();
            }
          }
          
          void set_brightness(int brightness) {
            brightness_ = brightness;
          }
          
          void set_colon(bool enabled) {
            colon_enabled_ = enabled;
            if (display_mode_ == 0) {
              displayTime();
            } else {
              displayDishwasherTime();
            }
          }
          
          void set_mode(int mode) {
            if (mode != display_mode_) {
              display_mode_ = mode;
              if (mode == 0) {
                status_ = "Displaying current time";
                displayTime();
              } else {
                status_ = "Displaying dishwasher completion time";
                update_dishwasher_time();
                displayDishwasherTime();
              }
            }
          }
          
          void update_dishwasher_time() {
            // Sprawdź, czy mamy informację o czasie zakończenia zmywarki
            if (id(text_dishwasher).has_state()) {
              std::string completion_time = id(text_dishwasher).state;
              // Parsowanie formatu "HH:MM"
              if (completion_time.length() >= 5 && completion_time[2] == ':') {
                try {
                  std::string hour_str = completion_time.substr(0, 2);
                  std::string minute_str = completion_time.substr(3, 2);
                  
                  dishwasher_hours_ = std::stoi(hour_str);
                  dishwasher_minutes_ = std::stoi(minute_str);
                  
                  ESP_LOGD("Projector", "Parsed dishwasher time: %02d:%02d", 
                           dishwasher_hours_, dishwasher_minutes_);
                } catch (...) {
                  ESP_LOGE("Projector", "Error parsing dishwasher time: %s", 
                           completion_time.c_str());
                }
              }
            }
          }
          
          void update_status() {
            std::string mode_text = (display_mode_ == 0) ? "Current time" : "Dishwasher time";
            int brightness_percent = (255 - brightness_) * 100 / 255;
            
            std::string status = mode_text + " mode";
            status += ", Brightness: " + std::to_string(brightness_percent) + "%";
            status += ", Colon: " + std::string(colon_enabled_ ? "ON" : "OFF");
            
            if (display_mode_ == 0) {
              char time_str[6];
              sprintf(time_str, "%02d:%02d", hours_, minutes_);
              status += ", Time: " + std::string(time_str);
            } else {
              char time_str[6];
              sprintf(time_str, "%02d:%02d", dishwasher_hours_, dishwasher_minutes_);
              status += ", Completion: " + std::string(time_str);
            }
            
            status_ = status;
            id(projector_status).publish_state(status);
          }
      
        private:
          GPIOPin *cs_pin_;
          GPIOPin *clk_pin_;
          GPIOPin *data_pin_;
          int hours_;
          int minutes_;
          int dishwasher_hours_;
          int dishwasher_minutes_;
          int brightness_;
          bool colon_enabled_;
          int display_mode_;
          std::string status_;
          
          const unsigned long comWr = 0b101;
          
          // Mapowanie cyfr na segmenty - segment 1 (a, b, c)
          uint8_t segm1(int digit) {
            uint8_t mapping[10] = {
                0b1111, // 0: colon/MSB + a, b, c
                0b1110, // 1: colon/MSB + b, c
                0b1011, // 2: colon/MSB + a, c
                0b1111, // 3: colon/MSB + a, b, c
                0b1110, // 4: colon/MSB + b, c
                0b1101, // 5: colon/MSB + a, b
                0b1101, // 6: colon/MSB + a, b
                0b1111, // 7: colon/MSB + a, b, c
                0b1111, // 8: colon/MSB + a, b, c
                0b1111  // 9: colon/MSB + a, b, c
            };
            return mapping[digit];
          }
          
          // Mapowanie cyfr na segmenty - segment 2 (d, e, f, g)
          uint8_t segm2(int digit) {
            uint8_t mapping[10] = {
                0b1101, // 0: d, e, f = 1, g = 0
                0b0000, // 1: none
                0b1110, // 2: d, e, g
                0b1010, // 3: d, g
                0b0011, // 4: f, g
                0b1011, // 5: d, f, g
                0b1111, // 6: d, e, f, g
                0b0000, // 7: none
                0b1111, // 8: d, e, f, g
                0b1011  // 9: d, f, g
            };
            return mapping[digit];
          }
          
          // Specjalne wartości dla trzeciej cyfry (dziesiątki minut)
          uint8_t tens_min_segm1(int digit) {
            uint8_t mapping[10] = {
                0b1111, // 0
                0b1110, // 1
                0b1011, // 2
                0b1111, // 3
                0b1110, // 4
                0b1101, // 5
                0b1101, // 6
                0b1111, // 7
                0b1111, // 8
                0b1111  // 9
            };
            return mapping[digit];
          }
          
          uint8_t tens_min_segm2(int digit) {
            uint8_t mapping[10] = {
                0b1101, // 0
                0b0000, // 1
                0b1110, // 2
                0b1010, // 3
                0b0011, // 4
                0b1011, // 5
                0b1111, // 6
                0b0000, // 7
                0b1111, // 8
                0b1011  // 9
            };
            return mapping[digit];
          }
      
          void lcdShift(unsigned long data) {
            unsigned long bit = 1;
            
            int i;
            int len = (int)(log(data)/log(2))+1;
            unsigned long chck = bit<<(len-1);
            
            this->cs_pin_->digital_write(false);
            
            for(i = 0; i < len; i++) {
              this->clk_pin_->digital_write(false);
              this->data_pin_->digital_write(((chck&(data<<i)) == chck) ? true : false);
              
              delayMicroseconds(85);
              this->clk_pin_->digital_write(true);
              delayMicroseconds(85);
            }
            
            this->cs_pin_->digital_write(true);
            delayMicroseconds(85);
          }
      
          void clearAll() {
            // Czyści wszystkie używane adresy
            lcdShift(comWr*1024 + 14*16 + 0);  // Jednostki godzin
            lcdShift(comWr*1024 + 15*16 + 0);
            lcdShift(comWr*1024 + 16*16 + 0);  // Dziesiątki godzin
            lcdShift(comWr*1024 + 17*16 + 0);
            lcdShift(comWr*1024 + 20*16 + 0);  // Dziesiątki minut
            lcdShift(comWr*1024 + 19*16 + 0);
            lcdShift(comWr*1024 + 22*16 + 0);  // Jednostki minut
            lcdShift(comWr*1024 + 21*16 + 0);
          }
      
          void displayTime() {
            // Oblicz cyfry dla każdej pozycji
            int dh = hours_ / 10;    // Dziesiątki godzin
            int uh = hours_ % 10;    // Jednostki godzin
            int dm = minutes_ / 10;  // Dziesiątki minut
            int um = minutes_ % 10;  // Jednostki minut
            
            // Wyczyść wyświetlacz przed wyświetleniem czasu
            clearAll();
            
            // Jednostki godzin (pozycja 1) - bez AM
            lcdShift(comWr*1024 + 14*16 + (segm1(uh) & 0b0111));
            lcdShift(comWr*1024 + 15*16 + segm2(uh));
            
            // Dziesiątki godzin (pozycja 0) - bez PM
            lcdShift(comWr*1024 + 16*16 + (segm1(dh) & 0b0111));
            lcdShift(comWr*1024 + 17*16 + segm2(dh));
            
            // Dziesiątki minut (pozycja 2) - używamy funkcji ze specjalnymi wartościami
            uint8_t dm_segm1_val = tens_min_segm1(dm);
            lcdShift(comWr*1024 + 20*16 + dm_segm1_val);
            lcdShift(comWr*1024 + 19*16 + tens_min_segm2(dm));
            
            // Jednostki minut (pozycja 3) - bez lower PM
            lcdShift(comWr*1024 + 22*16 + (segm1(um) & 0b0111));
            lcdShift(comWr*1024 + 21*16 + segm2(um));
            
            // Włącz/wyłącz dwukropek
            if (colon_enabled_) {
              lcdShift(comWr*1024 + 20*16 + (dm_segm1_val | 0b1000)); // Dodajemy bit MSB
            } else {
              lcdShift(comWr*1024 + 20*16 + (dm_segm1_val & 0b0111)); // Usuwamy bit MSB
            }
          }
          
          void displayDishwasherTime() {
            // Podobnie jak displayTime(), ale używa dishwasher_hours_ i dishwasher_minutes_
            int dh = dishwasher_hours_ / 10;    // Dziesiątki godzin
            int uh = dishwasher_hours_ % 10;    // Jednostki godzin
            int dm = dishwasher_minutes_ / 10;  // Dziesiątki minut
            int um = dishwasher_minutes_ % 10;  // Jednostki minut
            
            // Wyczyść wyświetlacz przed wyświetleniem czasu
            clearAll();
            
            // Jednostki godzin (pozycja 1) - bez AM
            lcdShift(comWr*1024 + 14*16 + (segm1(uh) & 0b0111));
            lcdShift(comWr*1024 + 15*16 + segm2(uh));
            
            // Dziesiątki godzin (pozycja 0) - bez PM
            lcdShift(comWr*1024 + 16*16 + (segm1(dh) & 0b0111));
            lcdShift(comWr*1024 + 17*16 + segm2(dh));
            
            // Dziesiątki minut (pozycja 2) - używamy funkcji ze specjalnymi wartościami
            uint8_t dm_segm1_val = tens_min_segm1(dm);
            lcdShift(comWr*1024 + 20*16 + dm_segm1_val);
            lcdShift(comWr*1024 + 19*16 + tens_min_segm2(dm));
            
            // Jednostki minut (pozycja 3) - bez lower PM
            lcdShift(comWr*1024 + 22*16 + (segm1(um) & 0b0111));
            lcdShift(comWr*1024 + 21*16 + segm2(um));
            
            // Włącz/wyłącz dwukropek - dla czasu zmywarki mrugamy dwukropkiem
            static bool blink_state = true;
            
            if (colon_enabled_) {
              // Dla trybu zmywarki, mrugający dwukropek
              blink_state = !blink_state;
              if (blink_state) {
                lcdShift(comWr*1024 + 20*16 + (dm_segm1_val | 0b1000)); // Dodajemy bit MSB
              } else {
                lcdShift(comWr*1024 + 20*16 + (dm_segm1_val & 0b0111)); // Usuwamy bit MSB
              }
            } else {
              lcdShift(comWr*1024 + 20*16 + (dm_segm1_val & 0b0111)); // Usuwamy bit MSB
            }
          }
      };
      
      // Piny dla projektora
      auto cs_pin = new GPIOOutputPin(GPIO_NUM_4);   // D2 na Wemos D1 Mini
      auto clk_pin = new GPIOOutputPin(GPIO_NUM_5);  // D1 na Wemos D1 Mini
      auto data_pin = new GPIOOutputPin(GPIO_NUM_14); // D5 na Wemos D1 Mini
      
      auto projector_display = new CeilingProjector(cs_pin, clk_pin, data_pin);
      return {projector_display};
