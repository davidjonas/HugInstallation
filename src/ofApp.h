#pragma once

#include "ofMain.h"
#include "ofxKinectTracker.h"
#include "ofxOpenCv.h"
#include "ofxCvHaarFinder.h"
#include "ofxGui.h"
#include "TemperatureMonitor.h"
#include "LightController.h"

class ofApp : public ofBaseApp{
public:

    void setup();
    void update();
    void updateLights();
    void handleStage();
    void draw();
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y );
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void exit();

    void saveSample(ofxKinectBlob blob);
    bool trainCascade();
    bool prepareData();
    void clearSamples();

    void activateFinder();
    void deactivateFinder();

    //TRACKER
    ofxKinectTracker tracker;

    //GUI
    void setupGUI();
    void drawGUI();
    void updateGUI();
		bool guiActive;
    ofxPanel calibrateGui;
    ofxFloatSlider threshold;
    ofxFloatSlider blurAmount;
    ofxToggle blur;
    ofxFloatSlider minDepth;
    ofxFloatSlider maxDepth;
    ofxFloatSlider minBlobSize;
    ofxFloatSlider edgeThreshold;
    ofxFloatSlider tolerance;
    ofxToggle captureBackground;
    ofxToggle showAll;
    ofxToggle trackColor;
    ofxVec2Slider kinect1;
    ofxVec2Slider kinect2;

    ofxPanel effectsGui;
    ofxFloatSlider speed;
    ofxFloatSlider backgroundSpeed;

    //HAAR FINDER and TRAINER
    ofxCvHaarFinder finder;
    ofImage frameTester;
    bool capturing;
    bool positiveSamples;
    bool finding;
    int samples;
    int negSamples;
    ofImage frame;
    ofFile infoFile;
    ofFile bgFile;

    //ARDUINO
    TemperatureMonitor tm;

    //LIGHTS
    LightController lights;
    int numPeople;
    int debugKey;
    int currentEffect = 0;
    float distance = 0;
    Stage stage;
    float symphonyTimer;
    float HugTimer;
    int currentSymphony;
    vector<vector<int>> symphonies;
    vector<int> symphonySpeeds;

    //calibration
    ofPoint k1Pos;
    ofPoint k2Pos;

};
