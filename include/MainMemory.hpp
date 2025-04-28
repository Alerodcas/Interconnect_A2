#ifndef MAINMEMORY_HPP
#define MAINMEMORY_HPP

#include <vector>
#include <cstdint>
#include <mutex>

class MainMemory {
public:
    MainMemory();

    std::vector<uint8_t> read(uint32_t addr, size_t size);
    void write(uint32_t addr, const std::vector<uint8_t>& data);

private:
    static constexpr size_t MEMORY_SIZE = 4096 * 4; // 4096 posiciones de 32 bits = 16KB
    std::vector<uint8_t> memory;
    std::mutex memMutex; // Para proteger acceso concurrente
};

#endif // MAINMEMORY_HPP
