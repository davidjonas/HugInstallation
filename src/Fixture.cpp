#include "Fixture.h"


Fixture::Fixture(uint8_t channel, uint8_t numChannels){
  this->channel = channel;
  this->numChannels = numChannels;
  for(uint8_t i=0; i<numChannels; i++)
  {
    values.push_back(0);
  }
  changed = false;
}

Fixture::~Fixture(){

}

void Fixture::update(){
  //... Not Necessary
}

bool Fixture::hasChanged(){
  if(changed)
  {
    changed = false;
    return true;
  }
  else {
    return false;
  }
}

uint8_t Fixture::getNumChannels(){
  return numChannels;
}

uint8_t Fixture::getChannel(){
  return channel;
}

void Fixture::setChannel(uint8_t channel)
{
  this->channel = channel;
}

uint8_t Fixture::get(uint8_t channel){
  if(channel < values.size())
  {
    return values[channel];
  }
  else {
    ofLogError("Fixture") << "Channel " << channel << " does not exist in this fixture. Returning 0...";
    return 0;
  }

}

void Fixture::set(uint8_t channel, uint8_t value, BlendMode b){
  if(channel < numChannels)
  {
    switch (b) {
      case SOLO:
        values[channel] = value;
        break;
      case ADD:
        if(values[channel]+value < 255) values[channel] += value;
        else values[channel] = 255;
        break;
      case SUB:
        if(values[channel]-value > 0) values[channel] -= value;
        else values[channel] = 0;
        break;
      case MULT:
      if(values[channel]*value < 255) values[channel] *= value;
      else values[channel] = 255;
        break;
    }

    changed = true;
  }
  else{
    ofLogError("Fixture") << "Fixture does not have channel " << channel;
  }
}

void Fixture::set(std::vector<uint8_t> values, BlendMode b){
  if(values.size() == this->values.size())
  {

    if (b == SOLO)
    {
      this->values = values;
    }
    else{
      for(uint8_t i=0; i<values.size(); i++)
      {
        set(i, values[i], b);
      }
    }
    changed = true;
  }
  else{
    ofLogError("Fixture") << "Values vector has the wrong size for this fixture ==> Size sent: " << values.size() << " Size needed: " << this->values.size();
  }
}


void Fixture::setAll(uint8_t value, BlendMode b)
{
  for(uint8_t i=0; i<values.size(); i++)
  {
    set(i, value, b);
  }
  changed = true;
}

std::vector<uint8_t> Fixture::getValues()
{
  return values;
}
