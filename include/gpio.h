#pragma once
#include "forward_declarations.h"

#include <iostream>
#include <string>
#include <pigpio.h>
#include <chrono>
#include <array>
#include <functional>


class GPIO : public std::ostream {
    public:
        enum PinType : int8_t {
            PIN_OUTPUT, PIN_INPUT, PIN_INPUT_PULLUP, PIN_INPUT_PULLDOWN
        };

        using Callback = std::function<void(int)>;

        static const std::array<const char*,4> StringType;

    private:
        int8_t pin;
        PinType type;
        Callback gpio_callback;

        static void gpioStaticAlert(int gpio, int level, uint32_t tick, void* _this);
        void gpioAlert(int gpio, int level, uint32_t tick);

    public:
        void updateMode();
        int read();
        bool write(int output);

        GPIO(int,PinType, Callback);
        virtual ~GPIO();
    
        inline friend std::ostream& operator<<(std::ostream& stream, const GPIO& obj) {
            stream << int(obj.pin) << "," << (obj.type == GPIO::PIN_OUTPUT ? "OUTPUT" : "INPUT") << (obj.type == GPIO::PIN_INPUT_PULLUP ? "_PU" : (obj.type == GPIO::PIN_INPUT_PULLDOWN ? "_PD" : ""));
            return stream;
        }
        
        friend class Zone;
    };