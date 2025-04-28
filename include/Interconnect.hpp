#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "Message.hpp"
#include "PE.hpp"
#include "MainMemory.hpp"
#include <unordered_map>


class PE; // Forward declaration

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    void start();
    void stop();
    void sendMessage(const Message& msg); // llamado por PEs
    void registerPE(uint8_t id, PE* pe);

private:
    void processLoop(); // hilo del interconnect

    MainMemory mainMemory;

    std::queue<Message> messageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool running = false;
    std::thread worker;
    std::unordered_map<uint8_t, PE*> peDirectory; // ID del PE → puntero al PE

};

#endif // INTERCONNECT_HPP
