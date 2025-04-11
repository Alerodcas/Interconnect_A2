#include "PE.hpp"
#include <fstream>
#include <iostream>

PE::PE(int id, uint8_t qos) : id(id), qos(qos) {}

void PE::loadInstructions(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string line;
    while (std::getline(file, line)) {
        instructionMemory.push_back(line);
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
    std::cout << "PE " << id << " (QoS " << (int)qos << ") ejecutando instrucciones:\n";
    for (const auto& instr : instructionMemory) {
        std::cout << " - " << instr << std::endl;
        // Aquí luego irá la lógica para generar mensajes al Interconnect
    }
}

int PE::getId() const {
    return id;
}

uint8_t PE::getQoS() const {
    return qos;
}
