#ifndef PE_HPP
#define PE_HPP

#include <string>
#include <vector>
#include <thread>
#include "CacheBlock.hpp"

class PE {
public:
    PE(int id, uint8_t qos);

    void loadInstructions(const std::string& filepath);
    void getInstructions();
    void start();
    void join();

    int getId() const;
    uint8_t getQoS() const;

private:
    void execute();  // función para el hilo
    void executeInstruction(const std::string& instruction);
    int id;
    uint8_t qos;
    std::vector<std::string> instructionMemory;

    std::thread thread;

    //Cache
    static constexpr int NUM_BLOCKS = 128;
    std::array<CacheBlock, NUM_BLOCKS> cache;

    void writeToCache(uint32_t addr, const std::vector<uint8_t>& data);
    std::vector<uint8_t> readFromCache(uint32_t addr, size_t size);
};

#endif // PE_HPP
