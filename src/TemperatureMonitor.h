#pragma once

#include "ofxSerial.h"
#include "ofxOpenCv.h"

//low range of the sensor
#define MINTEMP 16
//high range of the sensor
#define MAXTEMP 80
//Acknowledge byte
#define ACK 128
//Number of pixels on the thermal cam
#define AMG88xx_PIXEL_ARRAY_SIZE 64
#define TIMEOUT 1000

class TemperatureMonitor{
  private:
    ofx::IO::SerialDevice serialPort;
    std::vector<ofx::IO::SerialDeviceInfo> devicesInfo;
    int baudRate;
    float temperature;
    float humidity;
    float hottestSpot;
    ofxCvGrayscaleImage frameBuffer;
    bool running;
    bool finished;
    bool tempCycle;
    float timer;
    int interval;

    bool _readTemperature();
    bool _readFrame();
    void _start(int intervalInMilliseconds);

  public:
    bool connected;
    TemperatureMonitor();
    ~TemperatureMonitor();

    std::vector<ofx::IO::SerialDeviceInfo> getAvailableDevices();
    void begin(int index);
    void flush();
    void stop();
    float getTemperature();
    float getHumidity();
    float getHottestSpot();
    void update();
    ofxCvGrayscaleImage getFrame();
};
