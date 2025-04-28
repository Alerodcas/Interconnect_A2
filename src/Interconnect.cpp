#include "Interconnect.hpp"
#include <iostream>

Interconnect::Interconnect() {}

Interconnect::~Interconnect() {
    stop();
}

void Interconnect::start() {
    running = true;
    worker = std::thread(&Interconnect::processLoop, this);
}

void Interconnect::stop() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        running = false;
        cv.notify_all();
    }
    if (worker.joinable()) {
        worker.join();
    }
}

void Interconnect::sendMessage(const Message& msg) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        messageQueue.push(msg);
    }
    cv.notify_one();
}

void Interconnect::processLoop() {
    while (true) {
        Message msg;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return !messageQueue.empty() || !running; });

            if (!running && messageQueue.empty()) break;

            msg = messageQueue.front();
            messageQueue.pop();
        }

        // Procesar mensaje
        switch (msg.type) {
            case MessageType::READ_MEM:
                std::cout << "Interconnect: Procesa READ_MEM desde PE " << int(msg.src)
                          << " en dirección 0x" << std::hex << msg.addr
                          << " (" << std::dec << msg.size << " bytes)\n";
            break;
            case MessageType::WRITE_MEM:
                std::cout << "Interconnect: Procesa WRITE_MEM desde PE " << int(msg.src)
                          << " en dirección 0x" << std::hex << msg.addr
                          << " (" << std::dec << msg.data.size() << " bytes)\n";
            break;
            default:
                std::cout << "Interconnect: Mensaje tipo no implementado.\n";
        }
    }
}
