#include "Interconnect.hpp" // Incluye el archivo de encabezado de la clase Interconnect
#include <iostream>        // Para entrada/salida estándar (cout)
#include <mutex>           // Para la exclusión mutua al imprimir

extern std::mutex cout_mutex; // Mutex global definido en main.cpp
extern std::mutex cin_mutex; // Mutex global definido en main.cpp
extern bool stepByStep; // Condición externa de ejecución paso por paso

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
        messageQueue.push(msg);                      // Agrega el mensaje a la cola de mensajes
    }
    cv.notify_one(); // Notifica a un thread que esté esperando en la variable de condición que hay un nuevo mensaje
}

// Método para registrar un PE en el Interconnect
void Interconnect::registerPE(uint8_t id, PE* pe) {
    peDirectory[id] = pe; // Asocia el ID del PE con un puntero al objeto PE en el directorio de PEs
}

// Bucle principal de procesamiento de mensajes del Interconnect (ejecutado en un thread separado)
void Interconnect::processLoop() {
    while (true) { // Bucle infinito para procesar mensajes continuamente
        if (stepByStep) {
            {
                std::lock_guard<std::mutex> lock(cin_mutex);
                std::cin.get(); // Espera a que el usuario presione Enter
            }
        }

        Message msg; // Variable para almacenar el mensaje a procesar

        {
            std::unique_lock<std::mutex> lock(queueMutex); // Adquiere un unique lock para la cola de mensajes
            // Espera hasta que la cola de mensajes no esté vacía o el Interconnect se detenga
            cv.wait(lock, [this]() { return !messageQueue.empty() || !running; });

            // Si el Interconnect no está en ejecución y la cola de mensajes está vacía, sale del bucle
            if (!running && messageQueue.empty()) break;

            msg = messageQueue.front(); // Obtiene el mensaje del frente de la cola
            messageQueue.pop();         // Remueve el mensaje del frente de la cola
        }

        // Procesar el mensaje según su tipo
        switch (msg.type) {
            case MessageType::READ_MEM: { // Si el tipo de mensaje es READ_MEM (lectura de memoria)
                // Simula la lectura de la memoria principal
                auto data = mainMemory.read(msg.addr, msg.size);

                // Crear mensaje de respuesta READ_RESP
                Message response;
                response.type = MessageType::READ_RESP; // Establece el tipo de mensaje a READ_RESP
                response.src = 0xFF;                   // Establece la fuente como el Interconnect (valor arbitrario)
                response.addr = msg.addr;                 // La dirección de la respuesta es la misma que la solicitud
                response.data = data;                   // Los datos leídos de la memoria principal
                response.qos = msg.qos;                   // Propaga la QoS de la solicitud

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado READ_MEM PE " << int(msg.src)
                              << " Dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.size << " bytes)\n";
                }

                peDirectory[msg.src]->receiveResponse(response);
                peDirectory[msg.src]->handleResponses();

                break;
            }

            case MessageType::WRITE_MEM: { // Si el tipo de mensaje es WRITE_MEM (escritura en memoria)
                // Simula la escritura en la memoria principal

                mainMemory.write(msg.addr, msg.data);

                // Crear mensaje de respuesta WRITE_RESP
                Message response;
                response.type = MessageType::WRITE_RESP; // Establece el tipo de mensaje a WRITE_RESP
                response.src = 0xFF;                     // Establece la fuente como el Interconnect
                response.addr = msg.addr;                // La dirección de la respuesta es la misma que la solicitud
                response.size = 1;                       // Simula un estado OK (puedes definir códigos de error)

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado WRITE_MEM PE " << int(msg.src)
                              << " Dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.data.size() << " bytes)\n";
                }

                peDirectory[msg.src]->receiveResponse(response);
                peDirectory[msg.src]->handleResponses();

                break;
            }
            case MessageType::BROADCAST_INVALIDATE: { // Si el tipo de mensaje es BROADCAST_INVALIDATE

                uint8_t sourcePE = msg.src; // Guarda el ID del PE que originó el BROADCAST

                // Enviar INV_ACK a todos los PEs excepto al origen
                for (auto& [pe_id, pe_ptr] : peDirectory) {
                    if (pe_id != sourcePE) {
                        if (pe_ptr) {
                            pe_ptr->invalidateCacheLine(msg.addr);
                            Message invAck;
                            invAck.type = MessageType::INV_ACK;
                            invAck.src = pe_id; // La fuente del INV_ACK es el PE que lo envía
                            invAck.qos = pe_ptr->getQoS(); // Obtener la QoS del PE que responde (podría ser la del mensaje original)
                            // No es necesario incluir addr aquí, ya que el INV_ACK se refiere a la invalidación reciente
                            {
                                std::lock_guard<std::mutex> lock(cout_mutex);
                                std::cout << "IntConnect: Enviando INV_ACK a PE " << int(pe_id) << " Invalidación línea 0x" << std::hex << msg.addr << "\n";
                            }

                            pe_ptr->receiveResponse(invAck);
                            pe_ptr->handleResponses();

                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Procesado BROADCAST_INVALIDATE PE "
                  << int(msg.src) << ", Linea Caché " << std::hex << msg.addr << "\n";
                }

                // Después de enviar todas las invalidaciones y los INV_ACKs,
                // el Interconnect envía el INV_COMPLETE al PE que originó el BROADCAST_INVALIDATE.
                Message invComplete;
                invComplete.type = MessageType::INV_COMPLETE;
                invComplete.dest = sourcePE; // El destino es el PE que originó el BROADCAST
                invComplete.qos = peDirectory[sourcePE]->getQoS(); // Obtener la QoS del PE destino

                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "IntConnect: Enviando INV_COMPLETE a PE " << int(sourcePE) << " por invalidación de línea 0x" << std::hex << msg.addr << "\n";
                }

                peDirectory[sourcePE]->receiveResponse(invComplete);
                peDirectory[msg.src]->handleResponses();

                break;
            }

            default: // Si el tipo de mensaje no está implementado
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "IntConnect: Tipo de mensaje no implementado.\n";
            }
        }
    }
}
