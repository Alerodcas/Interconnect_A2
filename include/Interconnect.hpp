#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "Message.hpp"

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    void start();
    void stop();
    void sendMessage(const Message& msg); // llamado por PEs

private:
    void processLoop(); // hilo del interconnect

    std::queue<Message> messageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool running = false;
    std::thread worker;
};

#endif // INTERCONNECT_HPP
