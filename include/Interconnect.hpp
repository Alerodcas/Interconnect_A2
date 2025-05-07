#ifndef INTERCONNECT_HPP
#define INTERCONNECT_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>          // Para usar std::vector en la cola FIFO
#include <unordered_map>
#include "Message.hpp"
#include "PE.hpp"
#include "MainMemory.hpp"

class PE; // Forward declaration

// Estructura para comparar mensajes por QoS
struct CompareMessages {
    bool operator()(const Message& a, const Message& b) {
        return a.qos > b.qos;
    }
};

class Interconnect {
public:
    Interconnect();
    ~Interconnect();

    void start();
    void stop();
    void sendMessage(const Message& msg); // llamado por PEs
    void registerPE(uint8_t id, PE* pe);
    void join(); // Para esperar a que el hilo worker termine

private:
    void processLoop(); // hilo del interconnect
    void writeOutput(const std::string &line);

    std::string outputPath;

    MainMemory mainMemory;

    std::vector<Message> fifoMessageQueue;
    std::priority_queue<Message, std::vector<Message>, CompareMessages> priorityMessageQueue;
    std::mutex queueMutex;
    std::condition_variable cv;
    bool running = false;
    std::thread worker;
    std::unordered_map<uint8_t, PE*> peDirectory; // ID del PE → puntero al PE
};

#endif // INTERCONNECT_HPP