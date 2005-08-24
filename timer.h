#ifndef DNET_TIMER
#define DNET_TIMER

class Timer {
public:

	void waitForNextFrame();
	bool skipFrame();
    int framesBehind();
	void frameDone();

	long long ticksElapsed();
	long long getFrameTicks();

	Timer();

private:

	int frameNum;
	long long ticksPerFrame;
	long long ticksOffset;

};

#endif
