#include "Timer.h"

Timer::Timer() {
	//SetThreadAffinityMask() - helps to commit thread to one CPU
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	this->countsPerSecond = li.QuadPart;
	secondsByCount = 1.0 / this->countsPerSecond;//second in one count
}

void Timer::Reset() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	this->currTime = li.QuadPart;
	this->prevTime = this->currTime;
	this->deltaTime = 0;
}

double Timer::Tick() {
	this->prevTime = this->currTime;
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	this->currTime = li.QuadPart;
	this->deltaTime = this->currTime - this->prevTime;
	return this->deltaTime*this->secondsByCount;
}

LONGLONG Timer::GetCurrentCount() {
	return this->currTime;
}

double Timer::operator-(LONGLONG lastTime) {
	return (this->currTime - lastTime) * this->secondsByCount;
}

