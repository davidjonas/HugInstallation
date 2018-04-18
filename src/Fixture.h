#pragma once
#include "stdint.h"
#include "ofMain.h"
#include "config.h"

class Fixture {
  private:
    uint8_t channel;
    uint8_t numChannels;
    bool changed;
    std::vector<uint8_t> values;

  public:
    Fixture(uint8_t channel, uint8_t numChannels);
    ~Fixture();
    void update();
    bool hasChanged();
    uint8_t getNumChannels();
    uint8_t getChannel();
    uint8_t get(uint8_t channel);
    void set(uint8_t channel, uint8_t value, BlendMode b);
    void set(std::vector<uint8_t> values, BlendMode b);
    void setAll(uint8_t value, BlendMode b);
    std::vector<uint8_t> getValues();
};
