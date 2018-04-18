#pragma once

#include "ofxArtNetProtocol.h"
#include "Fixture.h"
#include "config.h"

class LightController {
  private:
    ofxArtnetProtocol artnet;
		uint8_t universe[512];
    vector<Fixture> fixtures;
    bool connected;
    bool running;
    int interval;
    void updateThread();
    void applyBackground();
    void applyEffects();

    //Interaction and hug properties
    Stage stage;

    //effects
    bool effectsOn;
    int background;
    std::vector<int> effects;
    float multiplier;
    int animation;
    float wave;
    float position;
    std::vector<int> loopOrder;


  public:
    float speed;
    float backgroundSpeed;

    LightController();
    ~LightController();

    bool connect(string ip_address);

    void update();

    void blackOut();
    void whiteOut();
    void setAllFixtures(vector<uint8_t> values, BlendMode b = SOLO);

    void addFixtures(int how_many, int channels_per_fixture, int start_at);
    uint8_t numFixtures();
    void setFixture(uint8_t index, vector<uint8_t> values, BlendMode b = SOLO);
    void setFixture(uint8_t index, uint8_t value, BlendMode b = SOLO);
    void setRGBFixture(uint8_t index, uint8_t red, uint8_t green, uint8_t blue, BlendMode b = SOLO);
    void setRGBWFixture(uint8_t index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white, BlendMode b = SOLO);
    void setWFixture(uint8_t index, uint8_t white, BlendMode b = SOLO);
    vector<uint8_t> getFixtureValues(uint8_t index);

    void start(float interval);
    void start();
    void stop();

    void setEffects(std::vector<int> eff);
    std::vector<int> getEffects();
    void addEffect(int eff);
    void clearEffects();
    void setBackground(int value);
    int getBackground();
    void setLoopOrder(std::vector<int> order);
    void setEffects(bool value);
    bool effectsAreOn();
};
