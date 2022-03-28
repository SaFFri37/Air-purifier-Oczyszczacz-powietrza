#include "settings.h"

void IRAM_ATTR encoderInterruptFunction(void* arg);
portMUX_TYPE encoderMux = portMUX_INITIALIZER_UNLOCKED;

enum EncDirection
{
    CW,
    CCW,
    None
};

class Encoder
{
private:
    uint8_t swPin;
    u_long lastPos;
    bool lastSw;

public:
    uint8_t clkPin;
    uint8_t dtPin;
    u_long lastStep;
    volatile int currentPos;

    Encoder(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    {
        this->clkPin = clkPin;
        this->dtPin = dtPin;
        this->swPin = swPin;

        this->lastSw = false;

        this->lastStep = 0;
        this->currentPos = 0;
        this->lastPos = 0;

        pinMode(this->clkPin, INPUT_PULLUP);
        pinMode(this->dtPin, INPUT_PULLUP);
        pinMode(this->swPin, INPUT_PULLUP);

        attachInterruptArg(this->clkPin, encoderInterruptFunction, this, CHANGE);
        attachInterruptArg(this->dtPin, encoderInterruptFunction, this, CHANGE);
    }

    EncDirection handleRotation()
    {
        if (this->currentPos > this->lastPos)
        {
            this->lastPos = this->currentPos;
            Serial.println((String)"Encoder CW: " + this->currentPos);
            return EncDirection::CW;
        }
        else if (this->currentPos < this->lastPos)
        {
            this->lastPos = this->currentPos;
            Serial.println((String)"Encoder CCW: " + this->currentPos);
            return EncDirection::CCW;
        }

        return EncDirection::None;
    }

    bool handleButton()
    {
        bool currentSw = !digitalRead(this->swPin);
        bool state = false;

        if (currentSw != this->lastSw && currentSw == true)
        {
            state = true;
            delay(5);
        }

        this->lastSw = currentSw;

        return state;
    }
};

void IRAM_ATTR encoderInterruptFunction(void* arg)
{
    Encoder* encoder = (Encoder*)arg;
    u_long currentTime = millis();
    u_long duration = currentTime - encoder->lastStep;
  
    if (duration >= DEBOUNCE_TIME)
    {
        encoder->lastStep = currentTime;

        portENTER_CRITICAL_ISR(&encoderMux);
        if (digitalRead(encoder->clkPin) == digitalRead(encoder->dtPin))
        {
            encoder->currentPos++;
        }
        else
        {
            encoder->currentPos--;
        }
        portEXIT_CRITICAL_ISR(&encoderMux);
    }
}
