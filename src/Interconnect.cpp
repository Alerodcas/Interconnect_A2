#include "Interconnect.hpp" // Incluye el archivo de encabezado de la clase Interconnect
#include "Utils.hpp"          // Archivo Funciones Adicionales
#include <iostream>         // Para entrada/salida estándar (cout)
#include <mutex>            // Para la exclusión mutua al imprimir
#include <queue>            // Para la cola de prioridad
#include <vector>           // Para usar std::vector en la cola FIFO
#include <algorithm>        // Para std::sort en la cola de prioridad
#include <fstream>

extern std::mutex cout_mutex; // Mutex global definido en main.cpp
extern std::mutex cin_mutex; // Mutex global definido en main.cpp
extern std::mutex execution; // Mutex global definido en main.cpp
extern int executionMode; // Modo de ejecución (0 -> FIFO, 1 -> Prioridad)

// Constructor por defecto de la clase Interconnect
Interconnect::Interconnect()
    : outputPath("../output/intconnect.txt")
{}

// Destructor de la clase Interconnect
Interconnect::~Interconnect() {
    stop(); // Llama al método stop para asegurar que el thread worker termine correctamente
}

// Método para iniciar el Interconnect
void Interconnect::start() {
    running = true;                                     // Marca el Interconnect como en ejecución
    worker = std::thread(&Interconnect::processLoop, this); // Crea un nuevo thread que ejecuta el método processLoop de este objeto Interconnect
}

// Método para detener el Interconnect
void Interconnect::stop() {
    running = false;                               // Marca el Interconnect como no en ejecución

    if (worker.joinable()) { // Verifica si el thread worker puede ser unido (si aún no ha terminado)
        worker.join();       // Espera a que el thread worker termine su ejecución
    }
}

// Método para enviar un mensaje al Interconnect
void Interconnect::sendMessage(const Message& msg) {
    {
        std::lock_guard<std::mutex> lock(queueMutex); // Adquiere un lock del mutex para proteger el acceso a la cola de mensajes
        if (executionMode == 1) { // Modo Prioridad
            priorityMessageQueue.push(msg);
        } else { // Modo FIFO (por defecto o si executionMode no es 1)
            fifoMessageQueue.push_back(msg);
        }
    }
    cv.notify_one(); // Notifica a un thread que esté esperando en la variable de condición que hay un nuevo mensaje
}

// Método para registrar un PE en el Interconnect
void Interconnect::registerPE(uint8_t id, PE* pe) {
    peDirectory[id] = pe; // Asocia el ID del PE con un puntero al objeto PE en el directorio de PEs
}

// Bucle principal de procesamiento de mensajes del Interconnect (ejecutado en un thread separado)
void Interconnect::processLoop() {
    while (true) { // Bucle mientras el Interconnect esté en ejecución
        Message msg; // Variable para almacenar el mensaje a procesar
        int arriveTransferTime;
        int sendTransferTime;

        {
            std::unique_lock<std::mutex> lock(queueMutex); // Adquiere un unique lock para la cola de mensajes
            // Espera hasta que haya mensajes en la cola apropiada o el Interconnect se detenga
            cv.wait(lock, [this]() {
                return (executionMode == 1 && !priorityMessageQueue.empty()) ||
                    (executionMode != 1 && !fifoMessageQueue.empty()) ||
                        !running;
            });

            // Si el Interconnect no está en ejecución y ambas colas están vacías, sale del bucle
            if (fifoMessageQueue.empty() && priorityMessageQueue.empty() && !running) break;

            if (executionMode == 1) { // Modo Prioridad
                msg = priorityMessageQueue.top();
                priorityMessageQueue.pop();
            } else { // Modo FIFO
                msg = fifoMessageQueue.front();
                fifoMessageQueue.erase(fifoMessageQueue.begin());
            }
        }

        {
            std::lock_guard<std::mutex> lock(cin_mutex);
            std::cin.get(); // Espera a que el usuario presione Enter
        }

        int peClock = peDirectory[msg.src]->getCycleCounter();
        clockCycle = std::max(clockCycle, peClock);

        // Procesar el mensaje según su tipo
        switch (msg.type) {
            case MessageType::READ_MEM: {

                // -------------------- Procesar Mensaje --------------------

                arriveTransferTime = 6 / BytesForCicle;
                if (arriveTransferTime == 0) arriveTransferTime = 1;

                clockCycle += arriveTransferTime;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado READ_MEM PE " << int(msg.src)
                                  << " Dirección 0x" << std::hex << msg.addr
                                  << " (" << std::dec << msg.size << " bytes)\n";
                    writeOutput( "READ_MEM 0 " +
                                std::to_string(6) + " P" + std::to_string(msg.src) + " " + std::to_string(clockCycle));
                }

                auto data = mainMemory.read(msg.addr, msg.size); // Obtener el bloque deseado de memoria

                // -------------------- Generar Respuesta --------------------

                clockCycle++;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviado READ_RESP a PE " << int(msg.src)
                                  << " Dirección 0x" << std::hex << msg.addr
                                  << " (" << std::dec << msg.size << " bytes)\n";
                    writeOutput( "READ_RESP 1 " +
                                std::to_string(6 + data.size()) + " P" + std::to_string(msg.src) + " " + std::to_string(clockCycle));
                }

                Message response;
                response.type = MessageType::READ_RESP;
                response.dest = msg.src;
                response.addr = msg.addr;
                response.data = data;
                response.qos = msg.qos;

                sendTransferTime = (6 + response.data.size()) / BytesForCicle;
                if (sendTransferTime == 0) sendTransferTime = 1;

                while (peDirectory[msg.src]->getCycleCounter() < clockCycle + sendTransferTime && !peDirectory[msg.src]->getComplete()) {waitUntilReady();}
                if (peDirectory[msg.src]->getComplete() == true) peDirectory[msg.src]->setCycleCounter(clockCycle + sendTransferTime);
                peDirectory[msg.src]->receiveResponse(response);
                peDirectory[msg.src]->handleResponses();

                break;
            }
            case MessageType::WRITE_MEM: {

                // -------------------- Procesar Mensaje --------------------

                int transferCycles = 6 + msg.data.size() / BytesForCicle;
                if (transferCycles == 0) transferCycles = 1;

                clockCycle += transferCycles;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado WRITE_MEM PE " << int(msg.src)
                                  << " Dirección 0x" << std::hex << msg.addr
                                  << " (" << std::dec << msg.data.size() << " bytes)\n";
                    writeOutput( "WRITE_MEM 0 " +
                                std::to_string(6 + msg.data.size()) + " P" + std::to_string(msg.src) + " " + std::to_string(clockCycle));
                }

                mainMemory.write(msg.addr, msg.data); // Escribir la información en Memoria

                // -------------------- Generar Respuesta --------------------

                clockCycle++;

                Message response;
                response.type = MessageType::WRITE_RESP;
                response.qos = msg.qos;
                response.dest = msg.src;
                response.status = true;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviado WRITE_RESP PE " << int(msg.src)
                                  << " Dirección 0x" << std::hex << msg.addr
                                  << " (Exito)\n";
                    writeOutput( "WRITE_RESP 0 " +
                                std::to_string(3) + " P" + std::to_string(msg.src) + " " + std::to_string(clockCycle));
                }

                sendTransferTime = 3 / BytesForCicle;
                if (sendTransferTime == 0) sendTransferTime = 1;

                while (peDirectory[msg.src]->getCycleCounter() < clockCycle + sendTransferTime && !peDirectory[msg.src]->getComplete()) {waitUntilReady();}

                peDirectory[msg.src]->receiveResponse(response);
                peDirectory[msg.src]->handleResponses();

                break;
            }
            case MessageType::BROADCAST_INVALIDATE: {

                int transferCycles = 6 / BytesForCicle;
                if (transferCycles == 0) transferCycles = 1;

                clockCycle += transferCycles;

                uint8_t sourcePE = msg.src;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado BROADCAST_INVALIDATE PE " << int(msg.src)
                                  << " Dirección 0x" << std::hex << msg.addr << "\n";
                    writeOutput( "BROADCAST_INVALIDATE 0 " +
                                std::to_string(6) + " P" + std::to_string(sourcePE) + " " + std::to_string(clockCycle));
                }

                clockCycle++;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviado INV_ACK a PE's Invalidación 0x" << std::hex << msg.addr << "\n";
                    writeOutput( "INV_ACK 1 " +
                                std::to_string(2) + " All " + std::to_string(clockCycle));
                }

                sendTransferTime = (2) / BytesForCicle;
                if (sendTransferTime == 0) sendTransferTime = 1;

                for (auto& [pe_id, pe_ptr] : peDirectory) {
                    if (pe_id != sourcePE && pe_ptr) {
                        pe_ptr->invalidateCacheLine(msg.addr);
                        Message invAck;
                        invAck.type = MessageType::INV_ACK;
                        invAck.src = pe_id;
                        invAck.qos = pe_ptr->getQoS();
                        while (pe_ptr->getCycleCounter() < clockCycle + sendTransferTime && !peDirectory[msg.src]->getComplete()) {waitUntilReady();}
                        pe_ptr->receiveResponse(invAck);
                        pe_ptr->handleResponses();
                    }
                }

                clockCycle++;

                Message invComplete;
                invComplete.type = MessageType::INV_COMPLETE;
                invComplete.dest = sourcePE;
                invComplete.qos = peDirectory[sourcePE]->getQoS();

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviando INV_COMPLETE a PE " << int(sourcePE) << " por invalidación de línea 0x" << std::hex << msg.addr << "\n";
                    writeOutput( "INV_ACK 1 " +
                                std::to_string(2) + " P" + std::to_string(sourcePE) + " " + std::to_string(clockCycle));
                }

                sendTransferTime = (2) / BytesForCicle;
                if (sendTransferTime == 0) sendTransferTime = 1;

                while (peDirectory[sourcePE]->getCycleCounter() < clockCycle + sendTransferTime && !peDirectory[msg.src]->getComplete()) {waitUntilReady();}

                peDirectory[sourcePE]->receiveResponse(invComplete);
                peDirectory[sourcePE]->handleResponses();
                break;
            }
            default: {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "IntConnect: Tipo de mensaje no implementado.\n";
            }
        }
    }
}

void Interconnect::writeOutput(const std::string& line) {
    std::ofstream file(outputPath, std::ios::app); // Abre el archivo en modo append
    if (file.is_open()) {
        file << line << '\n'; // Escribe la línea con salto de línea
    }
}

int Interconnect::getclockCycle() const {
    return clockCycle;
}
