#ifndef DNET_TIMER
#define DNET_TIMER

class Timer {
public:

	void waitForNextFrame();
	bool skipFrame();
	void frameDone();

	Timer();

private:

	int frameNum;
	long long ticksPerFrame;
	long long ticksOffset;

};

#endif
