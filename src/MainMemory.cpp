#include "MainMemory.hpp"
#include <algorithm>
#include <iostream>

MainMemory::MainMemory() {
    memory.resize(MEMORY_SIZE, 0);
}

std::vector<uint8_t> MainMemory::read(uint32_t addr, size_t size) {
    std::lock_guard<std::mutex> lock(memMutex);

    if (addr + size > memory.size()) {
        std::cerr << "ERROR: Lectura fuera de rango de memoria (addr = 0x"
                  << std::hex << addr << ", size = " << std::dec << size << ")\n";
        return {};
    }

    return std::vector<uint8_t>(memory.begin() + addr, memory.begin() + addr + size);
}

void MainMemory::write(uint32_t addr, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(memMutex);

    if (addr + data.size() > memory.size()) {
        std::cerr << "ERROR: Escritura fuera de rango de memoria (addr = 0x"
                  << std::hex << addr << ", size = " << std::dec << data.size() << ")\n";
        return;
    }

    std::copy(data.begin(), data.end(), memory.begin() + addr);
}
