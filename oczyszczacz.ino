#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <ClosedCube_HDC1080.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include "Encoder.h"
#include "settings.h"
#include "utils.h"

void wifiConnection();
void setRgbLed(uint8_t, uint8_t, uint8_t);
void setFanSpeed(uint8_t);
void readSensorsValue();
void updateMode(bool);
void updateFanSpeed(EncDirection);
double readPm25Sensor();
void displayData();
void updateRgbLed();

void serverMainPage();
void serverGetData();
void serverSetSpeed();
void serverSwitchMode();

const char mainPageHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en" data-theme="dark"> <head> <meta charset="UTF-8"> <meta name="viewport" content="width=device-width, initial-scale=1.0"> <link rel="stylesheet" href="https://unpkg.com/@picocss/pico@latest/css/pico.min.css"> <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script> <script src="https://code.jquery.com/ui/1.13.0/jquery-ui.min.js"></script> <title>Air Purifier</title> <style>h2, h3, label{text-align: center;}</style> </head> <body> <main class="container"> <div class="grid"> <div> <hgroup> <h2>Temperature</h2> <h3 id="temperature">0.0°C</h3> </hgroup> </div><div> <hgroup> <h2>Humidity</h2> <h3 id="humidity">0%</h3> </hgroup> </div><div> <hgroup> <h2>Pressure</h2> <h3 id="pressure">0.0 Pa</h3> </hgroup> </div></div><div class="grid"> <div> <hgroup> <h2>PM2.5</h2> <h3 id="pm25">0.0 mg/m<sup>3</sup></h3> </hgroup> </div><div> <hgroup> <h2>Fan speed</h2> <h3 id="fanspeed">0%</h3> </hgroup> </div><div> <hgroup> <h2>Mode</h2> <h3 id="mode">Auto</h3> </hgroup> </div></div><button style="width: 50%; margin-left: auto; margin-right: auto;" id="modebutton">Switch mode</button> <label> Fan speed <input type="range" min="0" max="255" value="0" id="fanslider"/> </label> </main> <script>$(document).ready(function(){function getData(){$.ajax('/getdata',{dataType: 'text', timeout: 240, success: function (data, status, xhr){let response=data.split(";"); $("#temperature").html(response[0] + "°C"); $("#humidity").html(response[1] + "%"); $("#pressure").html(response[2] + " Pa"); $("#pm25").html(response[3] + " mg/m<sup>3</sup>"); $("#fanspeed").html(response[4] + "%"); $("#mode").html(response[5]);}, error: function (jqXhr, textStatus, errorMessage){console.error(errorMessage);}});}setInterval(getData, 250); $("#fanslider").change(function(){$.post("/setspeed",{fanSpeed: $(this).val()});}); $("#modebutton").click(function(){$.get("/switchmode");});}); </script> </body></html>
)rawliteral";

double sensorsValue[Sensor::SENSORS_COUNT] = {0};
unsigned long lastUpdate = 0;
Mode mode = Mode::AUTO;
uint8_t fanSpeed = 0;
uint8_t lastFanSpeed = 0;
DustLevels dustLevels[NUM_OF_THRESH_LEVELS] = {{0.0, 25, 0, 255, 0}, {0.34, 128, 255, 255, 0}, {0.47, 255, 255, 0, 0}};

ClosedCube_HDC1080 hdc1080;
Adafruit_BMP280 bmp280;
Encoder encoder(ENC_CLK, ENC_DT, ENC_SW);
Adafruit_SH1106 display(SDA, SCL);
WebServer server(80);

void setup()
{
    Serial.begin(9600);

    // OLED setup
    display.begin(SH1106_SWITCHCAPVCC, OLED_ADRESS);

    // WiFi setup
    wifiConnection();

    // RGB LED setup
    ledcSetup(PwmChannels::PWM_LED_R, 5000, 8);
    ledcAttachPin(LED_R, PwmChannels::PWM_LED_R);
    ledcSetup(PwmChannels::PWM_LED_G, 5000, 8);
    ledcAttachPin(LED_G, PwmChannels::PWM_LED_G);
    ledcSetup(PwmChannels::PWM_LED_B, 5000, 8);
    ledcAttachPin(LED_B, PwmChannels::PWM_LED_B);
    setRgbLed(0, 0, 0);

    // HDC1080 setup
    hdc1080.begin(HDC_ADDRESS);

    // BMP280 setup
    if (!bmp280.begin(BMP_ADDRESS, BMP_CHIPID))
    {
        Serial.println("BMP280 setup error!");
        while (true) delay(100);
    }
    bmp280.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, 
                       Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

    // Fan setup
    ledcSetup(PwmChannels::PWM_FAN, 25000, 8);
    ledcAttachPin(FAN_PWM, PwmChannels::PWM_FAN);
    setFanSpeed(fanSpeed);

    // GP2Y1010AU0F setup
    pinMode(PM25_IR, OUTPUT);
    digitalWrite(PM25_IR, HIGH);

    // Server setup
    server.on("/", serverMainPage);
    server.on("/getdata", serverGetData);
    server.on("/setspeed", serverSetSpeed);
    server.on("/switchmode", serverSwitchMode);
    server.begin();
}

void loop()
{
    if (millis() - lastUpdate >= UPDATE_INTERVAL)
    {
        lastUpdate = millis();

        readSensorsValue();
        displayData();
        updateRgbLed();
    }

    updateMode(encoder.handleButton());
    updateFanSpeed(encoder.handleRotation());

    server.handleClient();

    delay(1);
}

void wifiConnection()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    display.setTextSize(1); 
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("WiFi setup...");
    display.display();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connecting to WiFi");
        delay(2000);
    }

    Serial.println(WiFi.localIP());

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println((String)"IP = " + WiFi.localIP().toString());
    display.display();
    delay(5000);
}

void setRgbLed(uint8_t red, uint8_t green, uint8_t blue)
{
    ledcWrite(PwmChannels::PWM_LED_R, red);
    ledcWrite(PwmChannels::PWM_LED_G, green);
    ledcWrite(PwmChannels::PWM_LED_B, blue);
}

void setFanSpeed(uint8_t speed)
{
    if (speed > 255) speed = 255;
    if (speed < 0) speed = 0;

    if (lastFanSpeed == 0 && speed > 0)
    {
        ledcWrite(PwmChannels::PWM_FAN, 255);
        delay(FAN_STARTUP_TIME);
    }

    if (lastFanSpeed != fanSpeed)
    {
        displayData();
    }

    lastFanSpeed = speed;

    ledcWrite(PwmChannels::PWM_FAN, speed);
}

void readSensorsValue()
{
    sensorsValue[Sensor::HDC1080_TEMPERATURE_S] = hdc1080.readTemperature();
    sensorsValue[Sensor::HDC1080_HUMIDITY_S] = hdc1080.readHumidity();

    sensorsValue[Sensor::BMP280_TEMPERATURE_S] = (double)bmp280.readTemperature();
    sensorsValue[Sensor::BMP280_PRESSURE_S] = (double)bmp280.readPressure();

    sensorsValue[Sensor::PM25_S] = readPm25Sensor();

    sensorsValue[Sensor::AVERAGE_TEMPERATURE] = (sensorsValue[Sensor::HDC1080_TEMPERATURE_S] + sensorsValue[Sensor::BMP280_TEMPERATURE_S]) / 2.0;
}

void updateMode(bool buttonState)
{
    if (buttonState)
    {
        mode = ((mode == Mode::AUTO) ? Mode::MANUAL : Mode::AUTO);
        displayData();
    }
}

void updateFanSpeed(EncDirection direction)
{
    if (mode == Mode::MANUAL)
    {
        if (fanSpeed < 255 && direction == EncDirection::CW)
        {
            fanSpeed++;
        }
        else if (fanSpeed > 0 && direction == EncDirection::CCW)
        {
            fanSpeed--;
        }
    }
    else if (mode == Mode::AUTO)
    {
        fanSpeed = 0;

        for (uint8_t i = 0; i < NUM_OF_THRESH_LEVELS; i++)
        {
            if (sensorsValue[Sensor::PM25_S] >= dustLevels[i].threshold)
            {
                fanSpeed = dustLevels[i].fanSpeed;
            }
        }
    }

    setFanSpeed(fanSpeed);
}

double readPm25Sensor()
{
    digitalWrite(PM25_IR, LOW);
    delayMicroseconds(280);
    uint16_t adcValue = analogRead(PM25_ANALOG_IN);
    digitalWrite(PM25_IR, HIGH);

    double voltage = (double)adcValue * (3.3 / (double)4096);
    double dust = (voltage - VOLTAGE_AT_0_MG_M3) * DUST_AT_1_V;

    return dust;
}

void displayData()
{
    display.clearDisplay();
    display.setTextSize(1); 
    display.setTextColor(WHITE);

    display.setCursor(0, 0);
    display.println((String)"T = " + sensorsValue[Sensor::AVERAGE_TEMPERATURE] + (char)247 + "C");

    display.setCursor(0, 10);
    display.println((String)"H = " + sensorsValue[Sensor::HDC1080_HUMIDITY_S] + "%");

    display.setCursor(0, 20);
    display.println((String)"P = " + sensorsValue[Sensor::BMP280_PRESSURE_S] + " Pa");

    display.setCursor(0, 30);
    display.println((String)"D = " + sensorsValue[Sensor::PM25_S] + " mg/m3");

    display.setCursor(0, 40);
    display.println((String)"F = " + (fanSpeed * 100 / 255) + "%");

    display.setCursor(0, 50);
    display.println((String)"M = " + (mode == Mode::MANUAL ? "Manual" : "Auto"));

    display.display();
}

void updateRgbLed()
{
    uint8_t red = 0, green = 0, blue = 0;

    for (uint8_t i = 0; i < NUM_OF_THRESH_LEVELS; i++)
    {
        if (sensorsValue[Sensor::PM25_S] >= dustLevels[i].threshold)
        {
            red = dustLevels[i].red;
            green = dustLevels[i].green;
            blue = dustLevels[i].blue;
        }
    }

    setRgbLed(red, green, blue);
}

void serverMainPage()
{
    server.send(200, "text/html", mainPageHtml);
}

void serverGetData()
{
    String response = "";
    response += (String)sensorsValue[Sensor::AVERAGE_TEMPERATURE] + ";";
    response += (String)sensorsValue[Sensor::HDC1080_HUMIDITY_S] + ";";
    response += (String)sensorsValue[Sensor::BMP280_PRESSURE_S] + ";";
    response += (String)sensorsValue[Sensor::PM25_S] + ";";
    response += (String)(fanSpeed * 100 / 255) + ";";
    response += (mode == Mode::MANUAL ? "Manual" : "Auto");

    server.send(200, "text/plain", response);
}

void serverSetSpeed()
{
    if (server.hasArg("fanSpeed"))
    {
        if (mode == Mode::MANUAL)
        {
            fanSpeed = server.arg("fanSpeed").toInt();
            setFanSpeed(fanSpeed);
        }

        server.send(200);
    }
    else
    {
        server.send(400);
    }
}

void serverSwitchMode()
{
    updateMode(true);
    server.send(200);
}
