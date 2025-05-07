#ifndef PE_HPP
#define PE_HPP

#include <string>
#include <vector>
#include <thread>
#include "CacheBlock.hpp"
#include "Message.hpp"
#include "Interconnect.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>

class Interconnect;

class PE {
public:
    PE(int id, uint8_t qos, Interconnect* interconnect);

    void loadInstructions(const std::string& filepath);
    void getInstructions();
    void start();
    void join();

    void receiveResponse(const Message& msg);
    void handleResponses();

    void invalidateCacheLine(uint32_t cache_line);

    void writeOutput(const std::string &line);

    int getId() const;
    uint8_t getQoS() const;

    int getCycleCounter() const;
    void setCycleCounter(int newClock);

private:
    void execute();  // función para el hilo
    void executeInstruction(const std::string& instruction);
    int id;
    uint8_t qos;
    Interconnect* interconnect;
    std::vector<std::string> instructionMemory;

    std::thread thread;

    std::string outputPath;

    //Cache
    static constexpr int NUM_BLOCKS = 128;
    std::array<CacheBlock, NUM_BLOCKS> cache;

    std::queue<Message> responseQueue;
    std::mutex responseMutex;
    std::condition_variable responseCV;

    void writeToCache(uint32_t addr, const std::vector<uint8_t>& data);
    std::vector<uint8_t> readFromCache(uint32_t addr, size_t size);

    int cycleCounter = 0; // Contador local de ciclos
};

#endif // PE_HPP
