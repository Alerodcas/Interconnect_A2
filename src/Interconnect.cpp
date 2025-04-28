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

void Interconnect::registerPE(uint8_t id, PE* pe) {
    peDirectory[id] = pe;
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

        // Procesar el mensaje
        switch (msg.type) {
            case MessageType::READ_MEM: {
                std::cout << "Interconnect: Procesando READ_MEM de PE " << int(msg.src)
                          << " dirección 0x" << std::hex << msg.addr
                          << " (" << std::dec << msg.size << " bytes)\n";

                auto data = mainMemory.read(msg.addr, msg.size);

                // Crear respuesta
                Message response;
                response.type = MessageType::READ_RESP;
                response.src = 0xFF; // Interconnect como origen
                response.addr = msg.addr;
                response.data = data;
                response.qos = msg.qos;

                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }

                break;
            }

            case MessageType::WRITE_MEM: {
                std::cout << "Interconnect: Procesando WRITE_MEM de PE " << int(msg.src)
                          << " dirección 0x" << std::hex << msg.addr
                          << " (" << std::dec << msg.data.size() << " bytes)\n";

                mainMemory.write(msg.addr, msg.data);

                // Crear respuesta
                Message response;
                response.type = MessageType::WRITE_RESP;
                response.src = 0xFF;
                response.addr = msg.addr;
                response.size = 1; // OK (1), podés luego usar 0 si fallara

                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }

                break;
            } case MessageType::BROADCAST_INVALIDATE: {
                std::cout << "Interconnect: Procesando BROADCAST_INVALIDATE desde PE "
                          << int(msg.src) << ", línea de caché " << std::hex << msg.addr << "\n";

                // Mandar a todos los PEs excepto al origen
                for (auto& [pe_id, pe_ptr] : peDirectory) {
                    if (pe_id != msg.src) {
                        if (pe_ptr) {
                            pe_ptr->invalidateCacheLine(msg.addr);
                        }
                    }
                }

                break;
            }


            default:
                std::cout << "Interconnect: Tipo de mensaje no implementado.\n";
        }
    }
}
