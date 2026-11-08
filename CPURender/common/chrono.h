#pragma once
#include <chrono>

class Chrono
{
private:
	std::chrono::steady_clock::time_point start;

public:
	void Start();
	float CurrentTime();
	void Restart();
};