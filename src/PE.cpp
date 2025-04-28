#include "PE.hpp"
#include <fstream>
#include <iostream>
#include <sstream> // para std::istringstream
#include <iomanip> // para std::hex

PE::PE(int id, uint8_t qos, Interconnect* interconnect)
    : id(id), qos(qos), interconnect(interconnect) {}

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

void PE::invalidateCacheLine(uint32_t cache_line) {
    if (cache_line >= NUM_BLOCKS) {
        std::cerr << "PE " << id << ": ERROR - Línea de caché inválida: " << cache_line << "\n";
        return;
    }

    cache[cache_line].valid = false;
    std::cout << "PE " << id << ": Línea de caché " << cache_line << " invalidada.\n";
}


void PE::executeInstruction(const std::string& instruction) {
    std::istringstream iss(instruction);
    std::string opcode;
    iss >> opcode;

    if (opcode == "READ_MEM") {
        std::string addr_str;
        size_t size;
        iss >> addr_str >> size;

        uint32_t addr = std::stoul(addr_str, nullptr, 16); // convierte hex a uint32_t

        // Primero revisa caché
        auto result = readFromCache(addr, size);
        if (!result.empty()) {
            std::cout << "PE " << id << " (QoS " << (int)qos << ") [CACHE HIT] Addr 0x"
                      << std::hex << addr << ", " << std::dec << size << " bytes.\n";
        } else {
            std::cout << "PE " << id << " (QoS " << (int)qos << ") [CACHE MISS] Addr 0x"
                      << std::hex << addr << " → Solicitando al Interconnect.\n";

            // Construir y enviar mensaje
            Message msg;
            msg.type = MessageType::READ_MEM;
            msg.src = id;
            msg.qos = qos;
            msg.addr = addr;
            msg.size = size;

            interconnect->sendMessage(msg);
        }

    } else if (opcode == "WRITE_MEM") {
        std::string addr_str;
        size_t num_lines;
        iss >> addr_str >> num_lines;

        uint32_t addr = std::stoul(addr_str, nullptr, 16);

        std::vector<uint8_t> fake_data(16, id); // Simulamos 16 bytes

        writeToCache(addr, fake_data);

        std::cout << "PE " << id << " escribió en caché Addr 0x" << std::hex << addr
                  << " (" << std::dec << num_lines << " líneas) y envía WRITE_MEM al Interconnect.\n";

        // Construir y enviar mensaje
        Message msg;
        msg.type = MessageType::WRITE_MEM;
        msg.src = id;
        msg.qos = qos;
        msg.addr = addr;
        msg.data = fake_data;

        interconnect->sendMessage(msg);

    } else if (opcode == "BROADCAST_INVALIDATE") {
        std::string cache_line_str;
        iss >> cache_line_str;

        uint32_t cache_line = std::stoul(cache_line_str, nullptr, 16);

        Message msg;
        msg.type = MessageType::BROADCAST_INVALIDATE;
        msg.src = id;
        msg.qos = qos;
        msg.addr = cache_line;

        interconnect->sendMessage(msg);

        std::cout << "PE " << id << " envió BROADCAST_INVALIDATE para línea 0x"
                  << std::hex << cache_line << "\n";
    } else {
        std::cout << "PE " << id << ": Instrucción desconocida → " << instruction << "\n";
    }
}

void PE::receiveResponse(const Message& msg) {
    {
        std::lock_guard<std::mutex> lock(responseMutex);
        responseQueue.push(msg);
    }
    responseCV.notify_one();
}

void PE::handleResponses() {
    std::unique_lock<std::mutex> lock(responseMutex);

    while (!responseQueue.empty()) {
        Message msg = responseQueue.front();
        responseQueue.pop();
        lock.unlock();

        if (msg.type == MessageType::READ_RESP) {
            std::cout << "PE " << id << ": Recibió READ_RESP, actualizando caché dirección 0x"
                      << std::hex << msg.addr << "\n";
            writeToCache(msg.addr, msg.data); // ✅ Actualiza caché
        }
        else if (msg.type == MessageType::WRITE_RESP) {
            std::cout << "PE " << id << ": Recibió WRITE_RESP, escritura confirmada\n";
        }
        else {
            std::cout << "PE " << id << ": Recibió tipo de mensaje inesperado\n";
        }

        lock.lock();
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
        handleResponses();
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
