#include "chrono.h"
#include <chrono>

void Chrono::Start()
{
	start = std::chrono::steady_clock::now();
}

float Chrono::CurrentTime()
{
	auto now = std::chrono::steady_clock::now();

	std::chrono::steady_clock::duration elapsed = now - start;

	return elapsed.count() / 1000000000.0f;
}

void Chrono::Restart()
{
	Start();
}