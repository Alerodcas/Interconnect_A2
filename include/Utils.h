#pragma once
#include <thread>
#include <chrono>

inline void waitUntilReady() { std::this_thread::sleep_for(std::chrono::microseconds(1));}