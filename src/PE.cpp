#include "PE.hpp"
#include <fstream>
#include <iostream>
#include <sstream> // para std::istringstream
#include <iomanip> // para std::hex

PE::PE(int id, uint8_t qos) : id(id), qos(qos) {}

void PE::loadInstructions(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        instructionMemory.push_back(line);
    }
}

void PE::getInstructions() {
    for (const auto& instr : instructionMemory) {
        std::cout << instr << "\n";
    }
}

void PE::executeInstruction(const std::string& instruction) {
    std::istringstream iss(instruction);
    std::string opcode;
    iss >> opcode;

    if (opcode == "READ_MEM") {
        std::string addr_str;
        size_t size;
        iss >> addr_str >> size;

        uint32_t addr = std::stoul(addr_str, nullptr, 16); // convierte de hex a int

        auto result = readFromCache(addr, size);
        if (!result.empty()) {
            std::cout << "PE " << id << " (QoS " << (int)qos << ") [CACHE HIT] Addr 0x"
                      << std::hex << addr << ", " << std::dec << size << " bytes.\n";
        } else {
            std::cout << "PE " << id << " (QoS " << (int)qos << ") [CACHE MISS] Addr 0x"
                      << std::hex << addr << " → Simular solicitud al Interconnect.\n";
            // Luego se genera un mensaje de READ_MEM para el Interconnect
        }

    } else if (opcode == "WRITE_MEM") {
        std::string addr_str;
        size_t num_lines;
        iss >> addr_str >> num_lines;

        uint32_t addr = std::stoul(addr_str, nullptr, 16);
        std::vector<uint8_t> fake_data(16, id); // datos de prueba: llenos con el ID del PE
        writeToCache(addr, fake_data);

        std::cout << "PE " << id << " escribió en caché Addr 0x" << std::hex << addr
                  << " (" << num_lines << " líneas) y simula envío a Interconnect.\n";

    } else if (opcode == "BROADCAST_INVALIDATE") {
        std::string cache_line;
        iss >> cache_line;

        std::cout << "PE " << id << " genera BROADCAST_INVALIDATE para línea "
                  << cache_line << ".\n";

    } else {
        std::cout << "PE " << id << ": Instrucción desconocida → " << instruction << "\n";
    }
}

void PE::start() {
    thread = std::thread(&PE::execute, this);
}

void PE::join() {
    if (thread.joinable())
        thread.join();
}

void PE::execute() {
    std::cout << "PE " << id << " (QoS " << (int)qos << ") iniciando ejecución...\n";
    for (const auto& instr : instructionMemory) {
        std::cout << "PE " << id << ": Instrucción → " << instr << "\n";
        executeInstruction(instr);
        // Aquí luego simular tiempo con stepping, NO sleep()
    }
}

//Lectura y escritura de cache
void PE::writeToCache(uint32_t addr, const std::vector<uint8_t>& data) {
    size_t blockIndex = (addr / 16) % NUM_BLOCKS;
    cache[blockIndex].tag = addr / 16;
    std::copy(data.begin(), data.begin() + std::min(data.size(), size_t(16)), cache[blockIndex].data.begin());
    cache[blockIndex].valid = true;
}

std::vector<uint8_t> PE::readFromCache(uint32_t addr, size_t size) {
    size_t blockIndex = (addr / 16) % NUM_BLOCKS;
    if (cache[blockIndex].valid && cache[blockIndex].tag == addr / 16) {
        return std::vector<uint8_t>(cache[blockIndex].data.begin(), cache[blockIndex].data.begin() + size);
    } else {
        // Simular un cache miss
        return {};
    }
}

int PE::getId() const {
    return id;
}

uint8_t PE::getQoS() const {
    return qos;
}
