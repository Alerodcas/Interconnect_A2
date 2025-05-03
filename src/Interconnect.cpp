#include "Interconnect.hpp" // Incluye el archivo de encabezado de la clase Interconnect
#include <iostream>        // Para entrada/salida estándar (cout)
#include <mutex>           // Para la exclusión mutua al imprimir

extern std::mutex cout_mutex; // Mutex global definido en main.cpp

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
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Interconnect: Procesando READ_MEM de PE " << int(msg.src)
                              << " dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.size << " bytes)\n";
                }

                // Simula la lectura de la memoria principal
                auto data = mainMemory.read(msg.addr, msg.size);

                // Crear mensaje de respuesta READ_RESP
                Message response;
                response.type = MessageType::READ_RESP; // Establece el tipo de mensaje a READ_RESP
                response.src = 0xFF;                   // Establece la fuente como el Interconnect (valor arbitrario)
                response.addr = msg.addr;                 // La dirección de la respuesta es la misma que la solicitud
                response.data = data;                   // Los datos leídos de la memoria principal
                response.qos = msg.qos;                   // Propaga la QoS de la solicitud

                // Enviar la respuesta al PE solicitante si está registrado
                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }

                break;
            }

            case MessageType::WRITE_MEM: { // Si el tipo de mensaje es WRITE_MEM (escritura en memoria)
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Interconnect: Procesando WRITE_MEM de PE " << int(msg.src)
                              << " dirección 0x" << std::hex << msg.addr
                              << " (" << std::dec << msg.data.size() << " bytes)\n";
                }

                // Simula la escritura en la memoria principal
                mainMemory.write(msg.addr, msg.data);

                // Crear mensaje de respuesta WRITE_RESP
                Message response;
                response.type = MessageType::WRITE_RESP; // Establece el tipo de mensaje a WRITE_RESP
                response.src = 0xFF;                    // Establece la fuente como el Interconnect
                response.addr = msg.addr;                  // La dirección de la respuesta es la misma que la solicitud
                response.size = 1;                      // Simula un estado OK (puedes definir códigos de error)

                // Enviar la respuesta al PE solicitante si está registrado
                if (peDirectory.count(msg.src)) {
                    peDirectory[msg.src]->receiveResponse(response);
                }

                break;
            }
            case MessageType::BROADCAST_INVALIDATE: { // Si el tipo de mensaje es BROADCAST_INVALIDATE
                {
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cout << "Interconnect: Procesando BROADCAST_INVALIDATE desde PE "
                              << int(msg.src) << ", línea de caché " << std::hex << msg.addr << "\n";
                }

                // Iterar a través de todos los PEs registrados
                for (auto& [pe_id, pe_ptr] : peDirectory) {
                    // Enviar la invalidación a todos los PEs excepto al que originó el mensaje
                    if (pe_id != msg.src) {
                        if (pe_ptr) {
                            pe_ptr->invalidateCacheLine(msg.addr);
                        }
                    }
                }

                // Aquí podrías implementar la lógica para enviar un INV_ACK de cada PE y luego un INV_COMPLETE al originador
                // Esto dependerá de la complejidad del protocolo de coherencia de caché que quieras simular.

                break;
            }


            default: // Si el tipo de mensaje no está implementado
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "Interconnect: Tipo de mensaje no implementado.\n";
            }
        }
    }
}
