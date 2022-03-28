typedef struct DustLevels
{
    double threshold;
    uint8_t fanSpeed;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} DustLevels;

enum PwmChannels
{
    PWM_LED_R,
    PWM_LED_G,
    PWM_LED_B,
    PWM_FAN,
    CHANNEL_4
};

enum Sensor
{
    HDC1080_TEMPERATURE_S,
    HDC1080_HUMIDITY_S,
    BMP280_TEMPERATURE_S,
    BMP280_PRESSURE_S,
    PM25_S,
    AVERAGE_TEMPERATURE,
    SENSORS_COUNT
};

enum Mode
{
    AUTO,
    MANUAL
};