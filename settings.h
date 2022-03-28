#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// RGB LED
#define LED_R 13
#define LED_G 14
#define LED_B 23

// Encoder
#define ENC_SW 17
#define ENC_DT 18
#define ENC_CLK 19
#define DEBOUNCE_TIME 20

// Fan
#define FAN_PWM 25
#define FAN_TACH 27
#define NUM_OF_THRESH_LEVELS 3
#define FAN_STARTUP_TIME 100

// GP2Y1010AU0F
#define PM25_ANALOG_IN 35
#define PM25_IR 16
#define VOLTAGE_AT_0_MG_M3 0.6
#define VOLTAGE_AT_0_5_MG_M3 3.6
#define DUST_AT_1_V (0.5 / (VOLTAGE_AT_0_5_MG_M3 - VOLTAGE_AT_0_MG_M3))

// HDC1080
#define HDC_ADDRESS 0x40

// BMP280
#define BMP_ADDRESS 0x76
#define BMP_CHIPID 0x58

// OLED
#define OLED_ADRESS 0x3C

#define UPDATE_INTERVAL 500
