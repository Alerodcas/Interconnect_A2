#include "Interconnect.hpp" // Incluye el archivo de encabezado de la clase Interconnect
#include <iostream>        // Para entrada/salida estándar (cout)
#include <mutex>           // Para la exclusión mutua al imprimir
#include <queue>           // Para la cola de prioridad
#include <vector>          // Para usar std::vector en la cola FIFO
#include <algorithm>       // Para std::sort en la cola de prioridad

extern std::mutex cout_mutex; // Mutex global definido en main.cpp
extern std::mutex cin_mutex; // Mutex global definido en main.cpp
extern std::mutex execution; // Mutex global definido en main.cpp
extern int executionMode; // Modo de ejecución (0 -> FIFO, 1 -> Prioridad)

// Constructor por defecto de la clase Interconnect
Interconnect::Interconnect() {}

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
    {
        std::unique_lock<std::mutex> lock(queueMutex); // Adquiere un unique lock para la cola de mensajes
        running = false;                               // Marca el Interconnect como no en ejecución
        cv.notify_all();                              // Notifica a todos los threads que estén esperando en la variable de condición
    }
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
    while (running) { // Bucle mientras el Interconnect esté en ejecución
        Message msg; // Variable para almacenar el mensaje a procesar

        {
            std::unique_lock<std::mutex> lock(queueMutex); // Adquiere un unique lock para la cola de mensajes
            // Espera hasta que haya mensajes en la cola apropiada o el Interconnect se detenga
            cv.wait(lock, [this]() {
                return (executionMode == 1 && !priorityMessageQueue.empty()) ||
                       (executionMode != 1 && !fifoMessageQueue.empty()) ||
                       !running;
            });

            // Si el Interconnect no está en ejecución y ambas colas están vacías, sale del bucle
            if (!running && fifoMessageQueue.empty() && priorityMessageQueue.empty()) break;

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

        // Procesar el mensaje según su tipo
        switch (msg.type) {
            case MessageType::READ_MEM: {
                auto data = mainMemory.read(msg.addr, msg.size);
                Message response;
                response.type = MessageType::READ_RESP;
                response.src = 0xFF;
                response.addr = msg.addr;
                response.data = data;
                response.qos = msg.qos;
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado READ_MEM PE " << int(msg.src)
                              << " Dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.size << " bytes)\n";
                }
                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }
                break;
            }
            case MessageType::WRITE_MEM: {
                mainMemory.write(msg.addr, msg.data);
                Message response;
                response.type = MessageType::WRITE_RESP;
                response.src = 0xFF;
                response.addr = msg.addr;
                response.size = 1;
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado WRITE_MEM PE " << int(msg.src)
                              << " Dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.data.size() << " bytes)\n";
                }
                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }
                break;
            }
            case MessageType::BROADCAST_INVALIDATE: {
                uint8_t sourcePE = msg.src;

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviado INV_ACK a PE's Invalidación línea 0x" << std::hex << msg.addr << "\n";
                }
                for (auto& [pe_id, pe_ptr] : peDirectory) {
                    if (pe_id != sourcePE && pe_ptr) {
                        pe_ptr->invalidateCacheLine(msg.addr);
                        Message invAck;
                        invAck.type = MessageType::INV_ACK;
                        invAck.src = pe_id;
                        invAck.qos = pe_ptr->getQoS();
                        pe_ptr->receiveResponse(invAck);
                    }
                }

                Message invComplete;
                invComplete.type = MessageType::INV_COMPLETE;
                invComplete.dest = sourcePE;
                invComplete.qos = peDirectory[sourcePE]->getQoS();
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviando INV_COMPLETE a PE " << int(sourcePE) << " por invalidación de línea 0x" << std::hex << msg.addr << "\n";
                }
                if (peDirectory.count(sourcePE)) {
                    peDirectory[sourcePE]->receiveResponse(invComplete);
                }
                break;
            }
            default: {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "IntConnect: Tipo de mensaje no implementado.\n";
            }
        }
    }
}
