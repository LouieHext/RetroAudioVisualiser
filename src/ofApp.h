#include "ofMain.h"
#include "ofxMaxim.h"
#include "ofxGui.h"

#include "maxiMFCC.h"

 
class ofApp : public ofBaseApp {

public:

	//oF
	~ofApp();
	void setup();
	void update();
	void draw();

	//setup helpers
	void guiSetup();
	void maxSetup();
	void visualsSetup();
	void palettesSetup();
	void getPalette();


	//calculators
	struct Result {
		float timeShortAverage; int minBin; int maxBin; float binFreq; float pitchCentroid;
	};

	Result getAudioFeatures();
	float fftLongAvg(float timeAverageShort);
	float maxPitchBinLongAverage(float maxPitchBin);
	bool beatDetctor(float timeAverageShort, float timeAverageLong);

	

	//visualisers
	void drawFFT(int minBin, int maxBin, float binFreq, bool onBeat);
	void drawOct();
	void drawInfo(float timeAverageShort);
	void drawShader();

	/* audio stuff */
	void audioOut(ofSoundBuffer& output) override; //output method
	void audioIn(ofSoundBuffer& input) override; //input method
	ofSoundStream soundStream;
	float* lAudioIn; /* inputs */
	float* rAudioIn;
	int		sampleRate;


	//MAXIMILIAN STUFF:
	double wave, sample, outputs[2], ifftVal;
	maxiMix mymix;
	maxiOsc osc;
	ofxMaxiFFTOctaveAnalyzer oct;

	int nAverages;
	float* ifftOutput;
	int ifftSize;

	float peakFreq = 0;
	float centroid = 0;
	float RMS = 0;

	double averageValue, pitchValue; 

	vector<float> timeAverage;
	vector<float> timeAveragePitch;
	vector<float> timeAverageRMS;

	ofxMaxiFFT mfft;
	int fftSize;
	int bins, dataSize;


	//GUI STUFF
	bool bHide;
	int beat;

	
	ofxPanel guiShader;
	ofxFloatSlider scale, timeScale, powerFBM,retro;
	ofxIntSlider numFBM;

	ofxPanel guiBeat;
	ofxFloatSlider minFreq,maxFreq,detectThreshold,DetectMultiplier;
	ofxIntSlider timeAverageLength,pitchAverageLength;
	ofxFloatSlider pitchMax, rmsMax;


	//visuals

	ofShader shader;
	ofPixels pixels;
	ofFbo canvasFbo;

	int x, y,offset;

	ofTrueTypeFont myfont, myfontBig;

	float horizWidth;
	float horizOffset;
	float vertOffset;
	int debugWidth, debugHeight;
	int maxPitch;

	//colours
	vector<glm::vec3>  paletteOne ;
	vector<glm::vec3> paletteTwo ;
	vector<glm::vec3> paletteThree;
	vector<glm::vec3> paletteFour;
	vector<glm::vec3> paletteFive;
	vector<glm::vec3> paletteSix;
	vector<glm::vec3> paletteSeven;
	vector<glm::vec3> paletteEight;

	vector <vector<glm::vec3>> palettes;

	int paletteID=0;

	float palette[3 * 8];


};

