#pragma once
#include "Config.h"

class Timer {
private:
	LONGLONG countsPerSecond;
	double secondsByCount;

	LONGLONG prevTime;
	LONGLONG currTime;
	LONGLONG deltaTime;
public:
	Timer();
	void Reset();
	double Tick();
	LONGLONG GetCurrentCount();
	double operator-(LONGLONG lastTime);
};