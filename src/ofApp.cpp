#include "ofApp.h"
#include "config.h"

//--------------------------------------------------------------
void ofApp::setup(){
	tracker.init();
	positiveSamples = true;
	capturing = false;
	finding = false;
	samples = 0;
	negSamples = 0;
	frame.allocate(tracker.getWidth(), tracker.getHeight(), OF_IMAGE_GRAYSCALE);
	frameTester.allocate(tracker.getWidth(), tracker.getHeight(), OF_IMAGE_GRAYSCALE);
	infoFile.open("info.dat",ofFile::Append);
	bgFile.open("bg.txt",ofFile::Append);
	stage = IDLE;

	currentSymphony = 0;
	symphonies.push_back({0,2,7,8});
	symphonySpeeds.push_back(10);

	symphonies.push_back({6, 7});
	symphonySpeeds.push_back(20);

	HugTimer = 0;
	symphonyTimer = 0;


	for(size_t i=0; i<tm.getAvailableDevices().size(); i++)
	{
		ofLogNotice("ofApp::setup") << "\t" << tm.getAvailableDevices()[i];

		std::size_t found = tm.getAvailableDevices()[i].getDescription().find("Arduino");

		if(found!=std::string::npos)
		{
			tm.begin(i);
			break;
		}
	}

	k1Pos.x = 0;
	k1Pos.y = 0;
	k2Pos.x = 640;
	k2Pos.y = 0;

	//LIGHTS
	lights.connect("192.168.0.254");
	//lights.addFixtures(1, 4, 48);
	lights.addFixtures(12, 4, 0);
	lights.setFixtureChannel(0, 48);
	lights.setEffects(true);
	lights.start();
	//lights.blackOut();
	//lights.whiteOut();
	numPeople = 0;

	setupGUI();
}

void ofApp::setupGUI()
{
	guiActive = false;
	calibrateGui.setup("Calibrate", "calibrate.xml", ofGetWindowWidth()-220, 10);
	calibrateGui.add(threshold.setup("Threshold", 5, 0.0, 255.0));
	calibrateGui.add(blurAmount.setup("Blur Amount", 9.0, 0.0, 100.0));
	calibrateGui.add(blur.setup("Blur", false));
	calibrateGui.add(minDepth.setup("Minimum Depth", 0.0, 0.0, 255.0));
	calibrateGui.add(maxDepth.setup("Maximum Depth", 255.0, 0.0, 255.0));
	calibrateGui.add(minBlobSize.setup("Minimum Blob Area", 100.0, 0.0, (tracker.getWidth()*tracker.getHeight())/2));
	calibrateGui.add(edgeThreshold.setup("Edge Threshold", 10.0, 0.0, 100.0));
	calibrateGui.add(tolerance.setup("Tracker tolerance", 50.0, 10.0, 500.0));
	calibrateGui.add(kinect1.setup("K1 position", ofVec2f(k1Pos.x, k1Pos.y), ofVec2f(k1Pos.x-200.0, k1Pos.x-100.0), ofVec2f(k1Pos.x+200.0, k1Pos.x+200.0)));
	calibrateGui.add(kinect2.setup("K2 position", ofVec2f(k2Pos.x, k2Pos.y), ofVec2f(k2Pos.x-200.0, k2Pos.x-100.0), ofVec2f(k2Pos.x+200.0, k2Pos.x+200.0)));
	calibrateGui.add(captureBackground.setup("Capture Background", false));
	calibrateGui.add(trackColor.setup("Track Color", false));
	calibrateGui.add(showAll.setup("Show all", false));

	calibrateGui.loadFromFile("calibrate.xml");

	effectsGui.setup("Effects", "effects.xml", ofGetWindowWidth()-220, 400);
	effectsGui.add(speed.setup("Speed", 1.0, 0.0, 20.0));
	effectsGui.add(backgroundSpeed.setup("Background Speed", 1.0, 0.0, 20.0));

	effectsGui.loadFromFile("effects.xml");
}

void ofApp::drawGUI() {
	if(guiActive)
  {
		calibrateGui.draw();
		effectsGui.draw();
	}
}

void ofApp::updateGUI() {
	tracker.setThreshold(threshold);
	tracker.setBlurAmount(blurAmount);
	tracker.setBlur(blur);
	tracker.setMinDepth(minDepth);
	tracker.setMaxDepth(maxDepth);
	tracker.setMinBlobSize(minBlobSize);
	tracker.setEdgeThreshold(edgeThreshold);
	tracker.setTolerance(tolerance);

	if(trackColor && tracker.getMode() == TRACK_DEPTH)
	{
		tracker.setMode(TRACK_COLOR);
	}

	if(!trackColor && tracker.getMode() == TRACK_COLOR)
	{
		tracker.setMode(TRACK_DEPTH);
	}

	if(captureBackground){
		captureBackground = false;
		tracker.grabBackground();
	}

	lights.speed = speed;
	lights.backgroundSpeed = backgroundSpeed;

	if(kinect1->x != k1Pos.x || kinect1->y != k1Pos.y)
	{
		k1Pos.x = kinect1->x;
		k1Pos.y = kinect1->y;

		tracker.calibratePosition(0, k1Pos);
	}

	if(kinect2->x != k2Pos.x || kinect2->y != k2Pos.y)
	{
		k2Pos.x = kinect2->x;
		k2Pos.y = kinect2->y;

		tracker.calibratePosition(1, k2Pos);
	}
}

bool ofApp::trainCascade(){
	string command = string("cd ../../../data; ./opencv_traincascade -data trainedData -vec positives.vec -bg bg.txt -w 24 -h 24 -numPos ")+ofToString(samples-1)+string(" -numNeg ")+ofToString(negSamples)+string(" -baseFormatSave");
	int value = system(command.c_str());
	if(value == 0)
	{
		activateFinder();
		return true;
	}
	return false;
}

bool ofApp::prepareData(){
	string command = string("cd ../../../data; ./opencv_createsamples -vec positives.vec -w 24 -h 24 -num ")+ofToString(samples)+string(" -info info.dat");
	int value = system(command.c_str());
	if(value == 0)
	{
		return true;
	}
	return false;
}

void ofApp::clearSamples(){
	infoFile.close();
	bgFile.close();
	system("cd ../../../data; rm -r positive/*; rm -r negative/*; echo \"\" > bg.txt; echo \"\" > info.dat");
	infoFile.open("info.dat",ofFile::Append);
	bgFile.open("bg.txt",ofFile::Append);
	samples = 0;
	negSamples = 0;
}

void ofApp::activateFinder(){
	finding = true;
	finder.setup("trainedData/cascade.xml");
}

void ofApp::deactivateFinder(){
	finding = false;
}

//--------------------------------------------------------------
void ofApp::update(){
	updateGUI();
	ofBackground(100,100,100);
	tracker.update();
	updateLights(); //TODO: Reactivate this.
	//lights.setBackground(1);

	if(ofGetElapsedTimef() > 20 && !tracker.getBackgroundSubtract())
	{
		tracker.grabBackground();
	}

	if(finding)
	{
		frameTester.setFromPixels(tracker.getGrayImage().getPixels());
		finder.findHaarObjects(frameTester);
	}
}

void ofApp::updateLights()
{
	int bl = tracker.getNumActiveBlobs();
	lights.clearEffects();

	if(bl > 1)
	{
		lights.setBackground(10);
		vector<ofxKinectBlob> blobs = tracker.getActiveBlobs();
		lights.follow = blobs[0].blob.centroid;
		distance = ofDist(blobs[0].blob.centroid.x, blobs[0].blob.centroid.y, blobs[1].blob.centroid.x, blobs[1].blob.centroid.y);
		float bgs = ofMap(distance, 300.0, 800.0, 20.0, 1.0);
		if(abs(bgs - backgroundSpeed) > 1)
		{
			backgroundSpeed = bgs;
		}
		if(distance < 300)
		{
				lights.setEffects({0, 1, 4});
		}
		else
		{
				lights.addEffect(4);
		}
	}
	else if(bl == 1)
	{
		vector<ofxKinectBlob> blobs = tracker.getActiveBlobs();
		lights.follow = blobs[0].blob.centroid;
		lights.setBackground(0);
		lights.addEffect(4);
	}
	else{
		lights.setBackground(-1);
		lights.addEffect(-1);
	}

	if(bl > 0)
	{
		if(tracker.thereAreOverlaps())
		{
			currentSymphony = floor(ofRandom(0, symphonies.size()));
			if(HugTimer == 0 && symphonyTimer == 0)
			{
				HugTimer = ofGetElapsedTimef();
			}
			else
			{
				if(HugTimer != 0 && ofGetElapsedTimef() > HugTimer + 3)
				{
					//HUGGED FOR 3 SECONDS Symphony starts
					symphonyTimer = ofGetElapsedTimef();
					HugTimer = 0;
				}
			}
			lights.setEffects(symphonies[currentSymphony]);
			speed = symphonySpeeds[currentSymphony];
			lights.setBackground(0);
		}

		if(symphonyTimer != 0 && symphonyTimer < 15)
		{
			lights.setEffects(symphonies[currentSymphony]);
			speed = symphonySpeeds[currentSymphony];
			lights.setBackground(0);
		}
		else if(symphonyTimer > 15)
		{
			symphonyTimer = 0;
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofBackground(40);
	ofSetHexColor(0xffffff);

	if(showAll)
	{
		//tracker.drawDebug(300,10);
		tracker.drawDebug(0,0);
	}
	else
	{
		// tracker.drawDepth(300,10);
		// tracker.drawContours(300,10);
		// tracker.drawBlobPositions(300,10);
		// tracker.drawEdgeThreshold(300, 10);

		tracker.drawDepth(0,0);
		tracker.drawContours(0,0);
		tracker.drawBlobPositions(0,0);
		tracker.drawEdgeThreshold(0,0);
	}


	//Haar finder stuff
	/*ofNoFill();
	if(positiveSamples)
	{
		ofSetHexColor(0x00ff00);
	}
	else{
		ofSetHexColor(0xff0000);
	}
	ofSetLineWidth(5);
	ofDrawRectangle(0,0,ofGetWindowWidth(),ofGetWindowHeight());
	ofSetLineWidth(1);

	if(capturing)
	{
		ofFill();
		ofSetHexColor(0xff0000);
		ofDrawEllipse(30, 30, 10, 10);

		for (int i = 0; i < tracker.blobs.size(); i++){
	    if(tracker.blobs[i].isActive())
	    {
				if(positiveSamples)
				{
					samples++;
				}
				else{
					negSamples++;
				}
				saveSample(tracker.blobs[i]);
			}
		}

		if(positiveSamples)
		{
			ofDrawBitmapString(samples, 50, 35);
		}
		else{
			ofDrawBitmapString(negSamples, 50, 35);
		}

	}

	if(finding)
	{
		for(unsigned int i = 0; i < finder.blobs.size(); i++) {
			ofSetHexColor(0xffff00);
			ofRectangle cur = finder.blobs[i].boundingRect;
			ofDrawRectangle(cur.x, cur.y, cur.width, cur.height);
		}
		ofSetHexColor(0xffff00);
		ofSetLineWidth(2);
		ofDrawRectangle(0,0,ofGetWindowWidth(),ofGetWindowHeight());
		ofSetLineWidth(1);
	}
	*/

	drawGUI();

	if(guiActive)
	{
		//Thermal
		ofSetColor(255);
		ofDrawBitmapString("Temperature: " + to_string(tm.getTemperature()), 50, 35);
		ofDrawBitmapString("Humidity: " + to_string(tm.getHumidity()), 50, 55);
		ofDrawBitmapString("Hottest Spot: " + to_string(tm.getHottestSpot()), 50, 75);
		ofSetHexColor(0xffffff);
		tm.getFrame().draw(50, 100, 200, 200);

		//Tracker
		ofNoFill();
		ofDrawRectangle(45, 350, 210, 50);
		ofDrawBitmapString("Persons: " + to_string(tracker.getNumActiveBlobs()), 50, 370);
		ofDrawBitmapString("Distance: " + to_string(distance), 50, 390);

		//effects
		if(lights.effectsAreOn())
		{
			ofDrawBitmapString("efects: ON", 50, ofGetWindowHeight() - 60);

		}
		else{
			ofDrawBitmapString("efects: OFF", 50, ofGetWindowHeight() - 60);
		}
		ofDrawBitmapString("background: " + to_string(lights.getBackground()), 50, ofGetWindowHeight() - 40);
		string effects = "Active effects: ";
		vector<int> ef = lights.getEffects();
		for(uint8_t i=0; i<ef.size(); i++)
		{
				effects += to_string(ef[i]);
				effects += ", ";
		}
		ofDrawBitmapString(effects, 50, ofGetWindowHeight() - 20);
	}

	if(!tracker.getBackgroundSubtract())
	{
		ofSetColor(255,0,0);
		ofNoFill();
		ofDrawRectangle(ofGetWindowWidth()/2-200, ofGetWindowHeight()/2-50, 400, 100);
		ofDrawBitmapString("DO NOT ENTER!", ofGetWindowWidth()/2-50, ofGetWindowHeight()/2-5);
		ofDrawBitmapString("NO ENTRAR!", ofGetWindowWidth()/2-50, ofGetWindowHeight()/2+8);
	}
}

void ofApp::saveSample(ofxKinectBlob blob){
	frame.setFromPixels(tracker.getGrayImage().getPixels());
	if(positiveSamples)
	{
		string filename = "positive/img_" + ofGetTimestampString() + ".jpg";
		frame.save(filename);
		infoFile << filename << " 1 " << blob.blob.boundingRect.getX() << " " << blob.blob.boundingRect.getY() << " " << blob.blob.boundingRect.getWidth() << " " << blob.blob.boundingRect.getHeight() << endl;
	}
	else
	{
		string filename = "negative/img_" + ofGetTimestampString() + ".jpg";
		frame.save(filename);
		bgFile << filename << endl;
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch (key){
		case ' ':
			tracker.grabBackground();
			break;
		/*case 'p':
				positiveSamples = !positiveSamples;
				break;
		case 'c':
				capturing = true;
				break;
		case 'f':
				if(finding){
					deactivateFinder();
				}
				else{
					activateFinder();
				}
				break;*/
		case '\t':
      guiActive = !guiActive;
      break;
		// case OF_KEY_RETURN:
				//if(prepareData()) trainCascade();
				//break;

		// case OF_KEY_DOWN:
		// 	k1Pos.y -= 1;
		// 	tracker.calibratePosition(0, k1Pos);
		// 	break;
		//
		// case OF_KEY_UP:
		// 	k1Pos.y += 1;
		// 	tracker.calibratePosition(0, k1Pos);
		// 	break;
		//
		// case OF_KEY_LEFT:
		// 	k1Pos.x -= 1;
		// 	tracker.calibratePosition(0, k1Pos);
		// 	break;
		//
		// case OF_KEY_RIGHT:
		// 	k1Pos.x += 1;
		// 	tracker.calibratePosition(0, k1Pos);
		// 	break;
		//
		// case 's':
		// 	k2Pos.y -= 1;
		// 	tracker.calibratePosition(1, k2Pos);
		// 	break;
		//
		// case 'w':
		// 	k2Pos.y += 1;
		// 	tracker.calibratePosition(1, k2Pos);
		// 	break;
		//
		// case 'a':
		// 	k2Pos.x -= 1;
		// 	tracker.calibratePosition(1, k2Pos);
		// 	break;
		//
		// case 'd':
		// 	k2Pos.x += 1;
		// 	tracker.calibratePosition(1, k2Pos);
		// 	break;
	}

	int bg = 0;
	switch(key)
	{
		case '-':
			lights.clearEffects();
			break;
		case '2':
			bg = lights.getBackground() == 10 ? 0 : lights.getBackground() + 1;
			lights.setBackground(bg);
			break;
		case '1':
			bg = lights.getBackground() == 0 ? 10 : lights.getBackground() - 1;
			lights.setBackground(bg);
			break;
		case '0':
			lights.addEffect(currentEffect);
			if(++currentEffect == 10)
			{
				currentEffect = 0;
			}
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
		switch (key){
			case 'c':
					//capturing = false;
					break;
		}

		debugKey = -1;
		//lights.clearEffects();
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	//lights.blackOut();

	//int fix = floor (ofMap(x, 0, ofGetWindowWidth(), 0, lights.numFixtures()));
	//lights.setFixture(fix, {0, 106, 85, 255});

	//uint8_t fix = floor (ofMap(x, 0, ofGetWindowWidth(), 0, 255));
	//lights.setAllFixtures({0, 106, 85, 255});
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	// if(button == 0)
	// {
	// 	tracker.grabBackground();
	// } else{
	// 	capturing = true;
	// }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	// if(button != 0)
	// {
	// 	capturing = false;
	// }
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}

void ofApp::exit(){
	ofLogNotice("HugInstallation") << "============== Bye Bye! ===============";
	lights.stop();
	tm.stop();

}
