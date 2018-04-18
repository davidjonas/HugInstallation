#include "TemperatureMonitor.h"

TemperatureMonitor::TemperatureMonitor(){
  baudRate = 9600;
  connected = false;
  devicesInfo = ofx::IO::SerialDeviceUtils::listDevices();
  temperature = 0;
  humidity = 0;
  hottestSpot = 0;
  frameBuffer.allocate(8,8);
  running = false;
  finished = true;
  tempCycle = true;
  timer = 0;
}

TemperatureMonitor::~TemperatureMonitor(){

}

std::vector<ofx::IO::SerialDeviceInfo> TemperatureMonitor::getAvailableDevices(){
  return devicesInfo;
}

void TemperatureMonitor::_start(int intervalInMilliseconds){
  running = true;
  finished = false;
  interval = intervalInMilliseconds;
  timer = ofGetElapsedTimef();
  std::thread t(&TemperatureMonitor::update, this);
  t.detach();
}

void TemperatureMonitor::begin(int index){
  if (!devicesInfo.empty())
    {
        // Connect to the first matching device.
        connected = serialPort.setup(devicesInfo[index], baudRate);

        if(connected)
        {
            ofLogNotice("ofApp::setup") << "Successfully setup " << devicesInfo[index];
            flush();
            _start(80.0f);
        }
        else
        {
            ofLogNotice("ofApp::setup") << "Unable to setup " << devicesInfo[index];
        }
    }
    else
    {
        ofLogNotice("ofApp::setup") << "No devices connected.";
    }
}

void TemperatureMonitor::flush(){
  std::chrono::milliseconds delay(1000);
  std::this_thread::sleep_for (delay);
  uint8_t buffer[512];
  ofLogNotice("TemperatureMonitor") << "Flushing...";
  while(serialPort.available())
  {
    serialPort.readBytes(buffer, 512);
    std::this_thread::sleep_for (delay);
    ofLogNotice("TemperatureMonitor") << "Flushed: " << buffer;
  }
}

void TemperatureMonitor::stop()
{
  running = false;
  while(!finished)
  {
    std::chrono::milliseconds smallDelay(10);
    std::this_thread::sleep_for (smallDelay);
  }
}

float TemperatureMonitor::getTemperature(){
  return temperature;
}

float TemperatureMonitor::getHumidity(){
  return humidity;
}

float TemperatureMonitor::getHottestSpot(){
  return hottestSpot;
}

bool TemperatureMonitor::_readTemperature()
{
  try
    {
        // Read all bytes from the device;
        uint8_t buffer[16];

        std::stringstream ss;
        bool t = false;
        bool h = false;

        if (serialPort.available() >= 16)
        {
            //ofLogNotice("TemperatureMonitor received") << serialPort.available() <<"  Reading data";
            std::size_t sz = serialPort.readBytes(buffer, 16);

            ofLogNotice("TemperatureMonitor") << "Received: "<< buffer;

            for (std::size_t i = 0; i < sz; ++i)
            {
                if(buffer[i] == '\n')
                {
                  if(ss.str()[0] == 't')
                  {
                    temperature = stof(ss.str().substr(1));
                    t = true;
                    ofLogNotice("TemperatureMonitor") << "parsed temperature: ";
                  } else if(ss.str()[0] == 'h')
                  {
                    humidity = stof(ss.str().substr(1));
                    h = true;
                    ofLogNotice("TemperatureMonitor") << "parsed humidity: ";
                  } else {
                    ofLogError("TemperatureMonitor") << "Found an unknown first character... ignoring";
                  }
                  ss=std::stringstream();
                  continue;
                }
                ss << buffer[i];
            }
            return t && h;
        }
        else{
          ofLogNotice("TemperatureAndHumidity") << "Not the right number of bytes: " << serialPort.available();
          return false;
        }
    }
    catch (const std::exception& exc)
    {
        ofLogError("TemperatureMonitor") << exc.what();
    }
}

bool TemperatureMonitor::_readFrame(){
  try
  {
    uint8_t frame[AMG88xx_PIXEL_ARRAY_SIZE];

    if(serialPort.available() >= AMG88xx_PIXEL_ARRAY_SIZE)
    {
      //ofLogNotice("Debug") << "Reading bytes: " << serialPort.available();
      serialPort.readBytes(frame, AMG88xx_PIXEL_ARRAY_SIZE);
      //ofLogNotice("Debug") << "Setting pixels";
      frameBuffer.setFromPixels(frame, 8, 8);
      frameBuffer.brightnessContrast(0.8f, 0.8f);

      hottestSpot = 0;
      for(size_t i=0; i<AMG88xx_PIXEL_ARRAY_SIZE; i++)
      {
        float pixelTemperature = ofMap(frame[i], 1, 254, MINTEMP, MAXTEMP);
        if(pixelTemperature > hottestSpot)
        {
          hottestSpot = pixelTemperature;
        }
      }
      return true;
    }
    else
    {
      //ofLogNotice("Frame") << "Not the right number of bytes: " << serialPort.available();
      return false;
    }
  }
  catch (const std::exception& exc)
  {
      ofLogError("FrameMonitor") << exc.what();
      return false;
  }
}

void TemperatureMonitor::update(){
  std::chrono::milliseconds smallDelay(10);
  std::chrono::milliseconds mill(interval);

  while(connected && running)
  {
    try{
        if(tempCycle)
        {
          //Ask for temp and humidity by sending byte 255
          serialPort.writeByte(255);
          int trys = 0;
          while (!_readTemperature())
          {
            std::this_thread::sleep_for (mill*10);
            if(trys % 20 == 0)
            {
              serialPort.writeByte(255);
            }
            trys++;
          }

          tempCycle = false;
          flush();
        }
        else {
          std::this_thread::sleep_for (mill);
          if(ofGetElapsedTimef() - timer > 60)
          {
            timer = ofGetElapsedTimef();
            ofLogNotice("TemperatureMonitor") << "Measuring temperature and humidity";
            flush();
            tempCycle = true;
          }

          //Ask for a frame of the thermal camera by sending byte 0
          serialPort.writeByte(0);
          std::this_thread::sleep_for (smallDelay);
          _readFrame();
        }
      }
      catch(const std::exception& exc)
      {
        ofLogError("FrameMonitor") << exc.what();
      }
  }
  ofLogNotice("Closing thermal data connection.");
  finished = true;
}

ofxCvGrayscaleImage TemperatureMonitor::getFrame(){
  return frameBuffer;
}
