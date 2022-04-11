#include "ofApp.h"
#include "maximilian.h" 


//created by Louie Hext - 03/2021


//Live Music Retro Visualiser

//based upon maxiFeature extraction code 
//domain warping from inigo Quilez - https://www.iquilezles.org/www/articles/warp/warp.htm
//beat detction algorithm from - https://en.wikipedia.org/wiki/Beat_detection
//dithering from - http://alex-charlton.com/posts/Dithering_on_the_GPU/

//this code extracts audio features which are then fed into a custom shader
//this shader uses dithering and domain warping to create a retro styled visual

//the "pitch centroid" of the sound is used to control the power of an FBM noise function in the shader
// higher pitches should correspond to smooth curves and lower more noisey ones

//the RMS value is used to control the scale of the visual. The louder the closer the visual will appear
// (as if youre "listening" to the visual

//a beat detection causes the dithering colour pallette to change

//these values are averaged over time to give some more stability. 


//to use adjust the min and max freq to the relevant domain that you wish to search for beats in
//adjust the detection multiplier to define the threshold (the magnitude in the domain needs to be
// greater than the previous timeaverage multiplied by the detection multiplier to count as a beat).

//adjust the max RMS of the music such that you the visuals pulse in and out
//adjust the mac pitch such that the visuals change from smooth -> noisey as the pitch changes

//these may need to be adjusted per music genre/song but mainly need to be changed based on audio setup




//-------------------------------------------------------------
ofApp::~ofApp() {
	//destructor
}



//--------------------------------------------------------------
void ofApp::setup() {

	//loaders
	shader.load("shadersGL3/shader");
	myfont.load("Minecraft.ttf", 12);
	myfontBig.load("Minecraft.ttf", 20);

	//set up helpers
	visualsSetup();
	maxSetup();
	guiSetup();
	palettesSetup();

}



//--------------------------------------------------------------
void ofApp::update() {
	//updated in case screen changes
	//reduced size to more clearly see dithering for retro feel
	x = (ofGetWidth() - debugWidth) / retro;
	y = ofGetHeight() / retro;


	//updating visual params
	scale = ofMap(pow(2,RMS), 0, rmsMax, 5.0, 0.01,true);
	powerFBM = ofMap(pitchValue, 2, pitchMax, 0.0, 1, true);

}



//--------------------------------------------------------------
void ofApp::draw() {

	//fps display
	ofSetColor(255);
	std::stringstream strm;
	strm << "fps: " << ofGetFrameRate();
	ofSetWindowTitle(strm.str());

	//getting audio results to calculate visual params from
	Result results = getAudioFeatures();
	pitchValue = maxPitchBinLongAverage(results.pitchCentroid); //time average pitch centroid
	float timeAverageLong = fftLongAvg(results.timeShortAverage); //time average FFT mag
	bool onBeat = beatDetctor(results.timeShortAverage, timeAverageLong); //beat detection

	//drawing calculation info
	drawFFT(results.minBin, results.maxBin, results.binFreq,onBeat); //fft spectrum
	drawOct(); //ocatve analyser
	drawInfo(results.timeShortAverage); //text
	
	//beat condition for new colour palette
	if (onBeat){
		getPalette();
	}
	
	//drawing shader visuals
	drawShader();

	//drawing GUIs
	guiBeat.draw();
	guiShader.draw();
	

}

//SET UP FUNCTIONS
//---------------------------------------------------------------------------
void ofApp::maxSetup() {

	//params
	sampleRate = 44100;
	int initialBufferSize = 512;

	//defining arrays
	lAudioIn = new float[initialBufferSize];
	rAudioIn = new float[initialBufferSize];
	memset(lAudioIn, 0, initialBufferSize * sizeof(float));
	memset(rAudioIn, 0, initialBufferSize * sizeof(float));


	//fft params
	fftSize = 1024 * 4; //larger value for more resolution when sampled
	mfft.setup(fftSize, 512, 256);
	nAverages = 12;
	oct.setup(sampleRate, fftSize / 2, nAverages);

	//maxi setup
	ofxMaxiSettings::setup(sampleRate, 2, initialBufferSize);


	//oF sound settings
	ofSoundStreamSettings settings;
	soundStream.printDeviceList();
	auto devices = soundStream.getMatchingDevices("default", ofSoundDevice::Api::MS_WASAPI);
	if (!devices.empty()) {
		settings.setInDevice(devices[0]);
	}

	settings.setInListener(this);
	settings.setOutListener(this);
	settings.sampleRate = sampleRate;
	settings.numOutputChannels = 2;
	settings.numInputChannels = 2;
	settings.bufferSize = initialBufferSize;
	settings.setApi(ofSoundDevice::Api::MS_WASAPI);

	soundStream.setup(settings);
}


void ofApp::guiSetup() {

	//gui for beat detection params
	guiBeat.setup();
	guiBeat.setName("Beat Detector");
	guiBeat.setSize(horizWidth, 100);
	guiBeat.setPosition(horizOffset, 120);
	guiBeat.loadFont("Minecraft.ttf", 12);
	guiBeat.setDefaultWidth(ofGetWidth()*0.2);

	//sliders
	guiBeat.add(minFreq.setup("minFreq", 0, 0, sampleRate / 10));
	guiBeat.add(maxFreq.setup("maxFreq", 10000, 0, 20000));
	guiBeat.add(detectThreshold.setup("detectThreshold", 0, 0, 0.5));
	guiBeat.add(DetectMultiplier.setup("DetectMultiplier", 1.5, 1.0, 5));
	guiBeat.add(timeAverageLength.setup("timeAverageLength", 60, 30, 300));
	guiBeat.add(pitchMax.setup("maxPitch", 80, 0, 200)); //you may need to change the upper limits on this
	guiBeat.add(rmsMax.setup("maxRMS", 3, 0,20 )); // and this, depending on audio set up
	guiBeat.add(pitchAverageLength.setup("timeAverageLength", 60, 30, 300));

	//gui for shader params (useful if you wish to manually explore)
	guiShader.setup();
	guiShader.setName("Visuals");
	guiShader.setSize(horizWidth, 120);
	guiShader.setDefaultWidth(ofGetWidth()*0.2);
	guiShader.setPosition(horizOffset, 330);
	guiShader.loadFont("Minecraft.ttf", 12);

	//sliders
	guiShader.add(scale.setup("scale", 0.45, 0.05, 5.0));
	guiShader.add(timeScale.setup("timeScale", 0.3, 0.01, 1.0));
	guiShader.add(powerFBM.setup("powerFBM", 0.1, 0.01, 0.4));
	guiShader.add(retro.setup("Retro", 2, 1, 10));
	guiShader.add(numFBM.setup("numFBM", 20, 1, 8));

}

void ofApp::visualsSetup(){
	//oF visuals
	ofBackground(50, 50, 50);
	ofSetBackgroundAuto(true);
	ofSetLineWidth(4);
	//parmas used in oF visuals
	debugWidth = ofGetWidth()*0.2;
	debugHeight = ofGetHeight();
	horizWidth = debugWidth * 0.8;
	horizOffset = debugWidth * 0.1;
	vertOffset = debugHeight * 0.7;
}
//---------------------------------------------------------------------------


//audio functions
//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& output) {
	//code adapted from maxiFeatureExtraction
	int outChannels = output.getNumChannels();
	for (int i = 0; i < output.getNumFrames(); ++i) {
		wave = lAudioIn[i];

		//get fft and octave

		if (mfft.process(wave)) {
			mfft.magsToDB();
			oct.calculate(mfft.magnitudesDB);

			float binFreq = 44100.0 / fftSize;
			float sumFreqs = 0;
			float sumMags = 0;
			float maxFreq = 0;
			int maxBin = 0;

			for (int i = 0; i < fftSize / 2; i++) {
				sumFreqs += (binFreq * i) * mfft.magnitudes[i];
				sumMags += mfft.magnitudes[i];
				if (mfft.magnitudes[i] > maxFreq) {
					maxFreq = mfft.magnitudes[i];
					maxBin = i;
				}
			}
			centroid = sumFreqs / sumMags; 
			peakFreq = (float)maxBin * (44100.0 / fftSize);

		}

		//no output
		output[i * outChannels] = 0;
		output[i * outChannels + 1] = 0;
	}
	
}


void ofApp::audioIn(ofSoundBuffer& input) {

    //getting audio in and calculating RMS
	float sum = 0;
	float count = 0;
	for (int i = 0; i < input.getNumFrames(); i++) {
		lAudioIn[i] = input[i * 2];
		rAudioIn[i] = input[i * 2 + 1];
		float sqr = input[i * 2] * input[i * 2];
		if (!isinf(sqr)) {
			sum += sqr * 1.0;
			count++;
		}
	}
	RMS = sqrt(sum / (count * 0.5));
	
	//calculating time averaged RMS
	if (timeAverageRMS.size() < int(10)) {
			if (!isinf(RMS) && !isnan(RMS)) {
				timeAverageRMS.push_back(RMS);
			}
		}
	if (timeAverageRMS.size() == int(10)) {
		timeAverageRMS.erase(timeAverageRMS.begin());
	}

	//summing up values in time average
	float timeAverageLong = 0.0;
	int c = 0;
	for (auto& n : timeAverage) {
		if (!isinf(n) && !isnan(n)) { //quick valid check
			timeAverageLong += n;
			c++;
		}
	}

	RMS = timeAverageLong / c;
}




//Calculations
//---------------------------------------------------------------------------

ofApp::Result ofApp::getAudioFeatures() {
	//extracts average FFT mag in the permitted freq domain
	//min and max FFT bins (for visual)
	//bin freq for displays
	//pitch centroid

	int size = 0;
	float sum = 0;
	int minBin = fftSize * 2;
	int maxBin = 0;
	float binFreq = float(sampleRate) / fftSize;

	//getting FFT mags and bin info
	for (int i = 0; i < fftSize / 2; ++i) {
		float freq = float(i)*binFreq; //freq to bin index
		if (freq > minFreq && freq < maxFreq) { //domain check
			float val = mfft.magnitudes[i];
			if (!isinf(val) && !isnan(val)) { //forcing only valid values
				size++;
				sum += val;
				if (i > maxBin) {
					maxBin = i;
				}
				if (i < minBin) {
					minBin = i;
				}
			}

		}
	}
	
	//getting pitch info
	float pitchCentroid = 0;
	float freqSum = 0;
	float valSum = 0;
	for (int i = 0; i < oct.nAverages; i++) {
		float val = oct.averages[i];
		ofLog() << " vak " << val*10;
		if (!isinf(val) && !isnan(val) && val>0.1) {
			freqSum += i;
			valSum += val;
		}
	}
	pitchCentroid = freqSum / valSum;
	return Result{ sum / size, minBin, maxBin, binFreq, pitchCentroid };

}




//function to calculate the time average of FFT values
float ofApp::fftLongAvg(float timeAverageShort) {
	//filling time average vector, fixed length (roughly last second, can extend)
	if (timeAverage.size() < int(timeAverageLength)) {
		if (!isinf(timeAverageShort) && !isnan(timeAverageShort)) {
			timeAverage.push_back(timeAverageShort);
		}
	}
	if (timeAverage.size() == int(timeAverageLength)) {
		timeAverage.erase(timeAverage.begin());
	}

	//summing up values in time average
	float timeAverageLong = 0.0;
	int c = 0;
	for (auto& n : timeAverage) {
		if (!isinf(n) && !isnan(n)) { //quick valid check
			timeAverageLong += n;
			c++;
		}
	}

	return timeAverageLong / c;
}

//function to calculate time average of pitch values
//should probably make this into the above funciton with some more generalisation
float ofApp::maxPitchBinLongAverage(float maxPitchBin) {
	//filling time average vector, fixed length (roughly last second, can extend)
	if (timeAveragePitch.size() < int(pitchAverageLength)) {
		if (!isinf(maxPitchBin) && !isnan(maxPitchBin)) {
			timeAveragePitch.push_back(maxPitchBin);
		}
	}
	if (timeAveragePitch.size() == int(pitchAverageLength)) {
		timeAveragePitch.erase(timeAveragePitch.begin());
	}

	//summing up values in time average
	float pitchAverageLong = 0.0;
	int c = 0;
	for (auto& n : timeAveragePitch) {
		if (!isinf(n) && !isnan(n)) { //quick valid check
			pitchAverageLong += n;
			c++;
		}
	}

	return pitchAverageLong / c;
}

//beat detection visuals and logic
bool ofApp::beatDetctor(float timeAverageShort, float timeAverageLong) {

	int gap = (horizWidth - 80) / 3;
	//this bar is the minimum power a frequency must have to be considred 
	//shown in red
	ofSetColor(255, 0, 0, 255);
	ofDrawRectangle(horizOffset, vertOffset + 50 - detectThreshold *50, 20, 3);

	//this bar is the current "average" from the previous samples
	//shown in green
	ofSetColor(0, 255, 0, 255);
	ofDrawRectangle(horizOffset + gap + 20, vertOffset + 50 - timeAverageLong *50, 20, 3);

	//this bar is the threshold for a beat to be considered
	//shown in white
	ofSetColor(255, 255, 255, 255);
	ofDrawRectangle(horizOffset + 2 * gap + 40, vertOffset + 50 - timeAverageLong *50 * DetectMultiplier, 20, 3);

	//this bar is the current average value in the allowed frequencies
	// shown in blue
	ofSetColor(0, 0, 255, 255);
	ofDrawRectangle(horizOffset + 3 * gap + 60, vertOffset + 50 - timeAverageShort *50, 20, 3);


	//beat condition
	if (timeAverageShort > DetectMultiplier*timeAverageLong && timeAverageShort > detectThreshold) {
		return true;
	}
	else {
		return false;
	}

}
//----------------------------------------------------------------


//drawing functions
//---------------------------------------------------------------------------------
void ofApp::drawFFT(int minBin, int maxBin, float binFreq, bool onBeat) {
	//visualing FFT (adapted from maxIFeature)
	if (onBeat) {
		ofSetColor(255);
	}
	else {
		ofSetColor(255, 0, 0);
	}

	ofPolyline line;
	int cc = 0;
	float xinc = horizWidth / (maxBin - minBin);
	for (int i = minBin; i < maxBin; ++i) {
		float freq = float(i)*binFreq;
		if (freq > minFreq && freq < maxFreq) {
			cc++;
			float height = mfft.magnitudes[i] *50;
			line.addVertex(ofPoint(horizOffset + (cc * xinc), vertOffset - height));
		}
	}
	line.draw();

}

//visualins octaves (from maxi feature)
void ofApp::drawOct() {
	ofSetColor(255, 0, 255, 200);
	float xinc = horizWidth / oct.nAverages;
	float maxPitchVal = 0;
	for (int i = 0; i < oct.nAverages; i++) {
		float val = oct.averages[i];
		if (!isinf(val) && !isnan(val)) {
			float height = val / 20.0 *50;
			ofDrawRectangle(horizOffset + (i * xinc), vertOffset - height + 150, 2, height);
		}
	}
}


void ofApp::drawInfo(float timeAverageShort) {
	//displaying infomation adpated from maxi feature
	ofSetColor(255);
	myfontBig.drawString("VISUALISER", horizOffset, 50);

	char avgString[255]; // an array of chars
	sprintf(avgString, "AVG: %.4f", timeAverageShort);
	myfont.drawString(avgString, horizOffset, vertOffset + 200);

	char minFreqStr[255]; // an array of chars
	sprintf(minFreqStr, "minFreq: %.0f", float(minFreq));
	myfont.drawString(minFreqStr, horizOffset, vertOffset + 230);

	char maxFreqStr[255]; // an array of chars
	sprintf(maxFreqStr, "maxFreq: %.0f", float(maxFreq));
	myfont.drawString(maxFreqStr, horizOffset, vertOffset + 260);


	char peakString[255]; // an array of chars
	sprintf(peakString, "Peak Freq: %.2f", peakFreq);
	myfont.drawString(peakString, horizOffset, vertOffset + 290);

	char centroidString[255]; // an array of chars
	sprintf(centroidString, "Spec Cent: %.2f", centroid);
	myfont.drawString(centroidString, horizOffset, vertOffset + 320);

	char pitchString[255]; // an array of chars
	sprintf(pitchString, "Pitch Cent: %.2f", pitchValue);
	myfont.drawString(pitchString, horizOffset, vertOffset + 350);


	char rmsString[255]; // an array of chars
	sprintf(rmsString, "RMS: %.2f", RMS);
	myfont.drawString(rmsString, horizOffset, vertOffset + 380);
}



//sends the unifroms to the shader and draws it to OF canvas
void ofApp::drawShader() {

	ofFbo fbo;
	fbo.allocate(x, y, GL_RGBA);
	fbo.begin();
	ofClear(0, 255);

	shader.begin();	

	shader.setUniform3fv("palette", palette ,8); //palette array
	shader.setUniform2f("u_resolution", x, y);
	shader.setUniform1f("u_time", ofGetFrameNum()*0.1);
	shader.setUniform1f("u_scale", scale);
	shader.setUniform1f("u_timeScale", timeScale);
	shader.setUniform1i("numFBM", numFBM);
	shader.setUniform1f("powerFBM", powerFBM);

	ofDrawRectangle(0, 0, x, y); //drawing

	shader.end();

	fbo.end();

	fbo.draw(debugWidth, 0, ofGetWidth() - debugWidth, ofGetHeight());
}
//----------------------------------------------------------------


//palette helpers
//----------------------------------------------------------------
void ofApp::getPalette() {
	//gets a random colour palette and fills the palette values with it
	paletteID = floor(ofRandom(0,8));
	vector<glm::vec3> colours = palettes[paletteID];
	for (int i = 0; i < 8; i++) {
		int index = i *3;
		palette[index] = float(colours[i][0])/255.0;
		palette[index+1] = float(colours[i][1])/255.0;
		palette[index+2] = float(colours[i][2])/255.0;
	}
}


//defines colour palletes
void ofApp::palettesSetup() {
	using namespace glm;

	//reds
	paletteOne.push_back(vec3(235,59,0));
	paletteOne.push_back(vec3(107,27,0));
	paletteOne.push_back(vec3(238,113,71));
	paletteOne.push_back(vec3(107,51,32));
	paletteOne.push_back(vec3(246,12,72));
	paletteOne.push_back(vec3(246,131,12));
	paletteOne.push_back(vec3(235, 127, 98));
	paletteOne.push_back(vec3(246,170,91));

	//greens
	paletteTwo.push_back(vec3(1, 107, 10));
	paletteTwo.push_back(vec3(74, 238, 87));
	paletteTwo.push_back(vec3(2, 235, 22));
	paletteTwo.push_back(vec3(33, 107, 39));
	paletteTwo.push_back(vec3(2, 184, 17));
	paletteTwo.push_back(vec3(150, 246, 10));
	paletteTwo.push_back(vec3(10, 246, 163));
	paletteTwo.push_back(vec3(117, 235 ,134));

	//blues
	paletteThree.push_back(vec3(144, 12, 246));
	paletteThree.push_back(vec3(3, 0, 107));
	paletteThree.push_back(vec3(74, 71, 238));
	paletteThree.push_back(vec3(3, 0, 235));
	paletteThree.push_back(vec3(33, 32, 107));
	paletteThree.push_back(vec3(3, 0, 184));
	paletteThree.push_back(vec3(12, 132, 246));
	paletteThree.push_back(vec3(138, 135 ,235));

	//yellows purple
	paletteFour.push_back(vec3(163, 161, 3));
	paletteFour.push_back(vec3(255, 251, 36));
	paletteFour.push_back(vec3(240, 236, 10));
	paletteFour.push_back(vec3(123, 50, 210));
	paletteFour.push_back(vec3(255, 255, 255));
	paletteFour.push_back(vec3(100, 0, 150));
	paletteFour.push_back(vec3(150, 115, 250));
	paletteFour.push_back(vec3(212, 50 ,240));

	//cyan orange
	paletteFive.push_back(vec3(164, 56, 0));
	paletteFive.push_back(vec3(2, 163, 152));
	paletteFive.push_back(vec3(115, 255, 245));
	paletteFive.push_back(vec3(84, 240, 229));
	paletteFive.push_back(vec3(164, 56, 0));
	paletteFive.push_back(vec3(240, 115, 53));
	paletteFive.push_back(vec3(167, 239, 240));
	paletteFive.push_back(vec3(240, 176, 137));
	
	//red green
	paletteSix.push_back(vec3(240, 1, 1));
	paletteSix.push_back(vec3(163, 149, 149));
	paletteSix.push_back(vec3(168, 240, 188));
	paletteSix.push_back(vec3(204, 255, 218));
	paletteSix.push_back(vec3(0, 163, 46));
	paletteSix.push_back(vec3(163, 80, 80));
	paletteSix.push_back(vec3(97, 255, 142));
	paletteSix.push_back(vec3(168, 128 ,120));

	//bnw
	paletteSeven.push_back(vec3(250, 250, 250));
	paletteSeven.push_back(vec3(200, 200, 200));
	paletteSeven.push_back(vec3(150, 150, 150));
	paletteSeven.push_back(vec3(100, 100, 100));
	paletteSeven.push_back(vec3(50, 50, 50));
	paletteSeven.push_back(vec3(0, 0, 0));
	paletteSeven.push_back(vec3(175, 175, 175));
	paletteSeven.push_back(vec3(125, 125 ,125));

	//all
	paletteEight.push_back(vec3(240, 217, 86));
	paletteEight.push_back(vec3(66, 245, 149));
	paletteEight.push_back(vec3(37, 17, 245));
	paletteEight.push_back(vec3(245, 50, 29));
	paletteEight.push_back(vec3(65, 245, 230));
	paletteEight.push_back(vec3(229, 245, 42));
	paletteEight.push_back(vec3(250, 250, 250));
	paletteEight.push_back(vec3(40, 40, 40));


	palettes.push_back(paletteOne);
	palettes.push_back(paletteTwo);
	palettes.push_back(paletteThree);
	palettes.push_back(paletteFour);
	palettes.push_back(paletteFive);
	palettes.push_back(paletteSix);
	palettes.push_back(paletteSeven);
	palettes.push_back(paletteEight);


	getPalette();
}
//----------------------------------------------------------------
