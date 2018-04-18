
#include "LightController.h"
#include <math.h>


LightController::LightController()
{
  running = false;
  interval = 40; //Defaults to 25 fps (1000 ms / 25 frames);
  blackOut();
  background = 0;
  stage = IDLE;
  effectsOn = true;
  speed = 1.0;
  backgroundSpeed = 1.0;
}

LightController::~LightController()
{
  stop();
}

bool LightController::connect(string ip_address){
    artnet.begin(ip_address.c_str());
    connected = true;

    return connected;
}

void LightController::update(){
  if(effectsOn)
  {
    applyBackground();
    applyEffects();
  }

  for(uint8_t i=0; i<fixtures.size(); i++)
  {
    fixtures[i].update();
    if(fixtures[i].hasChanged())
    {
      for(uint8_t c=0; c<fixtures[i].getNumChannels(); c++)
      {
        universe[fixtures[i].getChannel() + c] = fixtures[i].get(c);
      }
    }
  }
  artnet.send(universe, 0, 512);
}

void LightController::updateThread()
{
  while(running)
  {
    update();
    std::chrono::milliseconds mill(interval);
    std::this_thread::sleep_for (mill);
  }
}

void LightController::blackOut(){
  clearEffects();
  background = 0;

  for(uint8_t f=0; f<fixtures.size(); f++)
  {
    fixtures[f].setAll(0, SOLO);
  }

  for(int i=0; i<sizeof(universe); i++)
  {
    universe[i] = 0;
  }
}

void LightController::whiteOut(){
  for(int i=0; i<sizeof(universe); i++)
  {
    universe[i] = 255;
  }
}

void LightController::setAllFixtures(vector<uint8_t> values, BlendMode b)
{
  for(uint8_t i=0; i<numFixtures(); i++)
  {
    setFixture(i, values);
  }
}

void LightController::addFixtures(int how_many, int channels_per_fixture, int start_at){
  for(uint8_t i=start_at; i<(how_many*channels_per_fixture); i+=channels_per_fixture)
  {
    if(i < 512-channels_per_fixture)
    {
      Fixture f(i,channels_per_fixture);
      fixtures.push_back(f);
    }
    else{
      ofLogError("LightController") << "Fixture does not fit in the universe.";
    }
  }
}

uint8_t LightController::numFixtures(){
  return fixtures.size();
}

void LightController::setFixture(uint8_t index, vector<uint8_t> values, BlendMode b){
  if(index >= 0 && index < fixtures.size())
  {
    fixtures[index].set(values, b);
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
  }
}

void LightController::setFixture(uint8_t index, uint8_t value, BlendMode b){
  if(index >= 0 && index < fixtures.size())
  {
    fixtures[index].setAll(value, b);
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
  }
}

void LightController::setRGBFixture(uint8_t index, uint8_t red, uint8_t green, uint8_t blue, BlendMode b){
  if(index >= 0 && index < fixtures.size())
  {
    fixtures[index].set(RED_CHANNEL, red, b);
    fixtures[index].set(GREEN_CHANNEL, green, b);
    fixtures[index].set(BLUE_CHANNEL, blue, b);
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
  }
}

void LightController::setRGBWFixture(uint8_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white, BlendMode b){
  if(index >= 0 && index < fixtures.size())
  {
    fixtures[index].set(RED_CHANNEL, red, b);
    fixtures[index].set(GREEN_CHANNEL, green, b);
    fixtures[index].set(BLUE_CHANNEL, blue, b);
    fixtures[index].set(WHITE_CHANNEL, white, b);
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
  }
}

void LightController::setWFixture(uint8_t index, uint8_t white, BlendMode b){
  if(index >= 0 && index < fixtures.size())
  {
    fixtures[index].set(VALUE_CHANNEL, white, b);
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
  }
}

vector<uint8_t> LightController::getFixtureValues(uint8_t index)
{
  if(index >= 0 && index < fixtures.size())
  {
    return fixtures[index].getValues();
  }
  else{
    ofLogError() << "Fixture " << index << " does not exist.";
    return {};
  }
}

void LightController::start(float intervalInSeconds){
  running = true;
  interval = floor(intervalInSeconds * 1000);
  std::thread t(&LightController::updateThread, this);
  t.detach();
}

void LightController::start(){
  running = true;
  std::thread t(&LightController::updateThread, this);
  t.detach();
}

void LightController::stop(){
  clearEffects();
  blackOut();
  update();
  running = false;
}

void LightController::setEffects(std::vector<int> eff)
{
  effects = eff;
}

std::vector<int> LightController::getEffects()
{
  return effects;
}

void LightController::addEffect(int eff){
  effects.push_back(eff);
}

void LightController::clearEffects(){
  effects.clear();
}

void LightController::setBackground(int value){
  background = value;
}

int LightController::getBackground(){
  return background;
}

void LightController::setLoopOrder(std::vector<int> order){
  loopOrder = order;
}

void LightController::setEffects(bool value){
  effectsOn = value;
}

bool LightController::effectsAreOn()
{
  return effectsOn;
}

void LightController::applyBackground(){
  switch (background) {
    //===========================================================BACKGROUNDS==================================================================
    case 0:
      //========================NONE=============================
      //All OFF
      //MODE = SOLO;
      setAllFixtures({0,0,0,0});
      break;

    case 1:
      //========================IDLE=============================
      //All ON at 20% power
      //MODE = SOLO;
      multiplier = 0.2;
      setAllFixtures({(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      break;

    case 2:
      //========================LOW IDLE=============================
      //Fixtures 1 to 6 ON at 20% power all others OFF
      //MODE = SOLO;
      multiplier = 0.2;
      for(uint8_t i=0; i<12; i++)
      {
        if(i<6)
        {
          setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
        }
        else{
          setFixture(i, {0,0,0,0});
        }
      }
      break;

    case 3:
      //========================SUPER LOW IDLE=============================
      //Fixtures 7 to 10 ON at 20% power all others OFF
      //MODE = SOLO;
      multiplier = 0.2;
      for(uint8_t i=0; i<12; i++)
      {
        if(i<6 || i>9)
        {
          setFixture(i, {0,0,0,0});

        }
        else{
          setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
        }
      }
      break;

    case 4:
      //========================DIAGONAL IDLE=============================
      //Fixtures 11 and 12 ON at 20% power all others OFF
      //MODE = SOLO;

      setAllFixtures({0,0,0,0});

      multiplier = 0.2;
      setFixture(10, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      setFixture(11, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      break;

    case 5:
      //========================AMBIENT PULSE=============================
      //A soft random waving of all the lights, to create an organic ambient
      //MODE = SOLO;
      for(uint8_t i=0; i<numFixtures(); i++)
      {
        multiplier = ((cos((animation + i * 50)/20.0)/4) + 0.75) * 0.2;
        setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      }
      break;

    case 6:
      //========================AMBIENT PULSE HORIZONTAL=============================
      //A soft random waving of fixtures 1 to 6, to create an organic ambient
      //MODE = SOLO;
      for(uint8_t i=0; i<12; i++)
      {
        if(i<6)
        {
          multiplier = ((cos(((animation * backgroundSpeed) + i * 50)/20.0)/4) + 0.75) * 0.2;
          setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
        }
        else{
          setFixture(i, {0,0,0,0});
        }
      }
      break;


    case 7:
      //========================AMBIENT PULSE VERTICAL=============================
      //A soft random waving of fixtures 6 to 10, to create an organic ambient
      //MODE = SOLO;
      for(uint8_t i=0; i<12; i++)
      {
        if(i<6 || i>9)
        {
          setFixture(i, {0,0,0,0});
        }
        else{
          multiplier = ((cos(((animation * backgroundSpeed * 10) + i * 50)/20.0)/4) + 0.75) * 0.2;
          setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
        }
      }
      break;

    case 8:
      //========================AMBIENT PULSE DIAGONAL=============================
      //A soft random waving of fixtures 11 and 12, to create an organic ambient
      //MODE = SOLO;
      setAllFixtures({0,0,0,0});
      for(uint8_t i=10; i<12; i++)
      {
        multiplier = ((cos(((animation * backgroundSpeed * 10) + i * 50)/20.0)/4) + 0.75) * 0.2;
        setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      }
      break;

    case 9:
      //========================SPACED PULSE=============================
      //A full range random waving of all fixtures, to create an organic ambient
      //MODE = SOLO;
      for(uint8_t i=0; i<numFixtures(); i++)
      {
        if(i%2 == 0) multiplier = (sin(((animation * backgroundSpeed) + i * 135.890)/20.0));
        else multiplier = (cos(((animation * backgroundSpeed) + i * 100.20)/30.0));
        if(multiplier < 0) multiplier = 0;
        setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      }
      break;

    case 10:
      //========================BREATH=============================
      //A full range waving of all fixtures, to create an organic breathing ambient
      //MODE = SOLO;
      for(uint8_t i=0; i<numFixtures(); i++)
      {
        multiplier = ((cos(((animation * backgroundSpeed))/20.0)/4) + 0.75) * 0.7;
        setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)});
      }
      break;
    }
}

void LightController::applyEffects()
{
  for(uint8_t i=0; i<effects.size(); i++)
  {
    //===========================================================EFFECTS==================================================================
    switch (effects[i]) {
      case 0:
        //=======================FLASH GLITCH===========================
        //Random flashing of lights to create tension
        //MODE = ADD;
        multiplier = 1;
        if(animation % 3 == 0 && rand() % 100 + 1 > 80)
  			{
  				setFixture(floor(ofRandom(0, numFixtures()+1)), {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
  			}
        break;

      case 1:
          //=======================NEGATIVE FLASH GLITCH===========================
          //Random flashing of lights to create tension
          //MODE = SUB;
          multiplier = 1;
          if(animation % 3 == 0 && rand() % 100 + 1 > 80)
          {
            setFixture(floor(ofRandom(0, numFixtures()+1)), {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, SUB);
          }
          break;

      case 2:
        //=======================SPACE SWIPE===========================
        //A wave that goes back and forward in the space (fixtures 1 to 6)
        //MODE = ADD;
        wave = cos(((animation * speed)/150.0)) * 2;
  			position = 6 * wave;
  			for(uint8_t i=0; i<6; i++)
  			{
  				multiplier = ofMap(abs(i-position), 0.0f, 3.0f, 1.0f, 0.0f);
  				if(multiplier < 0) multiplier = 0;
  				if(multiplier > 1) multiplier = 1;
  				setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
  			}
        break;

      case 3:
          //=======================SPACE SWIPE VERTICAL===========================
          //A wave that goes back and forward in the space (fixtures 7 to 10)
          //MODE = ADD;
          wave = sin(((animation+20 * speed)/150.0)) * 2;
          position = (3 * wave) + 6;
          for(uint8_t i=6; i<9; i++)
          {
            multiplier = ofMap(abs(i-position), 0.0f, 3.0f, 1.0f, 0.0f);
            if(multiplier < 0) multiplier = 0;
            if(multiplier > 1) multiplier = 1;
            setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
          }
          break;
      case 4:
          //=======================ROOM ROTATE===========================
          //Uses fixtures 11 and 12 alternated with 2 and 4 and 7 and 9 to create the ilusion of rotation in the room; Use fast;
          //MODE = ADD;
          break;
      case 5:
          //=======================ORDERED LOOP===========================
          //Uses the cached value loopOrder to light up the fixtures in order, fading through them.
          //MODE = ADD;
          wave = cos(((animation * speed)/150.0)) * 2;
    			position = loopOrder.size() * wave;
    			for(uint8_t i=0; i<loopOrder.size(); i++)
    			{
    				multiplier = ofMap(abs(i-position), 0.0f, 3.0f, 1.0f, 0.0f);
    				if(multiplier < 0) multiplier = 0;
    				if(multiplier > 1) multiplier = 1;
    				setFixture(loopOrder[i], {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
    			}
          break;
      case 6:
          //=======================FULL RANDOM FLASH===========================
          //All fixtures flash at the same time;
          //MODE = ADD;
          multiplier = 1;
          if(animation % 3 == 0 && rand() % 100 + 1 > 98)
    			{
    				setAllFixtures({(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
    			}
          break;
      case 7:
          //=======================HALF RANDOM FLASH===========================
          //Flashes fixtures [1-6] or [7-10]
          //MODE = ADD;
          multiplier = 1;
          if(animation % 3 == 0 && rand() % 100 + 1 > 80)
    			{
            if(rand() % 100 + 1 > 50)
            {
              for(uint8_t i=6; i<10; i++)
              {
        				setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
              }
            }
            else
            {
              for(uint8_t i=0; i<6; i++)
              {
        				setFixture(i, {(uint8_t)floor(0*multiplier), (uint8_t)floor(106*multiplier), (uint8_t)floor(85*multiplier), (uint8_t)floor(255*multiplier)}, ADD);
              }
            }
    			}
          break;
          break;
      case 8:
          //=======================MIDDLE SWIPE LOW===========================
          //Swipes from the middle of [1-6] (3) twards 0
          //MODE = ADD;
          break;
      case 9:
          //=======================MIDDLE SWIPE HIGH===========================
          //Swipes from the middle of [1-6] (3) twards 6
          //MODE = ADD;
          break;
    }
  }
  animation += 1;
}
