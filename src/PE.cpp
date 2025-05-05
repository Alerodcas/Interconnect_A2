#include "PE.hpp" // Incluye el archivo de encabezado de la clase PE
#include <fstream>  // Para trabajar con archivos (lectura de instrucciones)
#include <iostream> // Para entrada/salida estándar (cout, cerr)
#include <sstream>  // Para manipular strings como streams (istringstream para parsear instrucciones)
#include <iomanip>  // Para formatear la salida (ej: std::hex para hexadecimal)
#include <mutex>           // Para la exclusión mutua al imprimir

extern std::mutex cout_mutex; // Mutex global definido en main.cpp
extern std::mutex cin_mutex; // Mutex global definido en main.cpp
extern bool stepByStep; // Condición externa de ejecución paso por paso

// Constructor de la clase PE
PE::PE(int id, uint8_t qos, Interconnect* interconnect)
    : id(id),                 // Inicializa el ID del PE con el valor proporcionado
      qos(qos),               // Inicializa la calidad de servicio (QoS) del PE
      interconnect(interconnect) {} // Inicializa el puntero al objeto Interconnect

// Método para cargar las instrucciones desde un archivo
void PE::loadInstructions(const std::string& filepath) {
    std::ifstream file(filepath); // Abre el archivo especificado en modo lectura
    std::string line;             // Variable para almacenar cada línea leída del archivo
    while (std::getline(file, line)) { // Lee el archivo línea por línea
        instructionMemory.push_back(line); // Agrega cada línea (instrucción) al vector instructionMemory
    }
}

// Método para imprimir las instrucciones cargadas (principalmente para tests)
void PE::getInstructions() {
    for (const auto& instr : instructionMemory) { // Itera a través de cada instrucción en instructionMemory
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << instr << "\n"; // Imprime la instrucción seguida de una nueva línea
        }
    }
}

// Método para invalidar una línea específica de la caché del PE
void PE::invalidateCacheLine(uint32_t cache_line) {
    // Verifica si el índice de la línea de caché está dentro de los límites válidos
    if (cache_line >= NUM_BLOCKS) {
        std::cerr << "PE " << id << ": ERROR - Línea Caché Inválida: " << "\n";
        return; // Sale de la función si la línea de caché es inválida
    }

    cache[cache_line].valid = false; // Marca la línea de caché como inválida
    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "PE " << id << ": Línea Caché " << cache_line << " Invalidada.\n";
    }

}

// Método para ejecutar una instrucción individual
void PE::executeInstruction(const std::string& instruction) {
    std::istringstream iss(instruction); // Crea un stringstream para parsear la instrucción
    std::string opcode;                 // Variable para almacenar el código de operación (la primera palabra de la instrucción)
    iss >> opcode;                      // Lee el primer token (opcode) del stringstream

    // Si el opcode es "READ_MEM" (operación de lectura de memoria)
    if (opcode == "READ_MEM") {
        std::string addr_str; // String para almacenar la dirección (en formato hexadecimal)
        size_t size;          // Tamaño de los datos a leer

        iss >> addr_str >> size; // Lee la dirección y el tamaño del stringstream

        uint32_t addr = std::stoul(addr_str, nullptr, 16); // Convierte la dirección hexadecimal a un entero sin signo de 32 bits

        // Primero revisa la caché
        auto result = readFromCache(addr, size); // Intenta leer los datos de la caché
        if (!result.empty()) { // Si el resultado no está vacío (cache hit)
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id <<  ": Encontrado CACHE HIT Addr 0x"
                          << std::hex << addr << ", " << std::dec << size << " bytes.\n";
            }
        } else { // Si la lectura de la caché devuelve un vector vacío (cache miss)

            // Construir y enviar mensaje de READ_MEM al Interconnect
            Message msg;
            msg.type = MessageType::READ_MEM; // Establece el tipo de mensaje a READ_MEM
            msg.src = id;                     // Establece la fuente del mensaje como el ID del PE
            msg.qos = qos;                     // Establece la calidad de servicio del mensaje
            msg.addr = addr;                   // Establece la dirección de memoria a leer
            msg.size = size;                   // Establece el tamaño de los datos a leer

            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Encontrado CACHE MISS Addr 0x"
                          << std::hex << addr << "\n";
                std::cout << "PE " << id << ": Solicitud READ_MEM a IntConnect Addr 0x"
                          << std::hex << addr << "\n";
            }

            interconnect->sendMessage(msg); // Envía el mensaje al Interconnect
        }

    }
    // Si el opcode es "WRITE_MEM" (operación de escritura en memoria)
    else if (opcode == "WRITE_MEM") {
        std::string addr_str;   // String para almacenar la dirección (en formato hexadecimal)
        size_t num_lines;       // Número de líneas de caché a escribir (aunque actualmente no se usa directamente para el tamaño de los datos simulados)
        iss >> addr_str >> num_lines; // Lee la dirección y el número de líneas del stringstream

        uint32_t addr = std::stoul(addr_str, nullptr, 16); // Convierte la dirección hexadecimal a un entero sin signo de 32 bits
        std::vector<uint8_t> fake_data;

        for (size_t i = 0; i < num_lines; ++i) {
            std::vector<uint8_t> fake_data(16, id); // Simula 16 bytes de datos para escribir, llenándolos con el ID del PE
            writeToCache(addr + i, fake_data);       // Escribe los datos simulados en la caché
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "PE " << id << ": Escritura Linea Caché Addr 0x" << std::hex << addr
                    << " (" << std::dec << num_lines << "\n";
            std::cout << "PE " << id << ": Solicitud WRITE Addr 0x" << std::hex << addr
                    << " (" << std::dec << num_lines << " Linea) \n";
        }

        // Construir y enviar mensaje de WRITE_MEM al Interconnect
        Message msg;
        msg.type = MessageType::WRITE_MEM; // Establece el tipo de mensaje a WRITE_MEM
        msg.src = id;                      // Establece la fuente del mensaje como el ID del PE
        msg.qos = qos;                      // Establece la calidad de servicio del mensaje
        msg.addr = addr;                    // Establece la dirección de memoria a escribir
        msg.data = fake_data;              // Establece los datos a escribir

        interconnect->sendMessage(msg); // Envía el mensaje al Interconnect

    }
    // Si el opcode es "BROADCAST_INVALIDATE" (operación de invalidación de caché)
    else if (opcode == "BROADCAST_INVALIDATE") {
        std::string cache_line_str; // String para almacenar la línea de caché a invalidar (en formato hexadecimal)
        iss >> cache_line_str;      // Lee la línea de caché del stringstream

        uint32_t cache_line = std::stoul(cache_line_str, nullptr, 16); // Convierte la línea de caché hexadecimal a un entero sin signo de 32 bits

        // Construir y enviar mensaje de BROADCAST_INVALIDATE al Interconnect
        Message msg;
        msg.type = MessageType::BROADCAST_INVALIDATE; // Establece el tipo de mensaje a BROADCAST_INVALIDATE
        msg.src = id;                                 // Establece la fuente del mensaje como el ID del PE
        msg.qos = qos;                                // Establece la calidad de servicio del mensaje
        msg.addr = cache_line;                        // Establece la línea de caché a invalidar

        interconnect->sendMessage(msg); // Envía el mensaje al Interconnect

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "PE " << id << " Solicitud Broadcast Invalidade Addr 0x"
                    << std::hex << cache_line << "\n";
        }
    }
    // Si el opcode no coincide con ninguna instrucción conocida
    else {
        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "PE " << id << ": Instrucción desconocida → " << instruction << "\n";
        }
    }
}

// Método para recibir un mensaje de respuesta del Interconnect
void PE::receiveResponse(const Message& msg) {
    // Bloque protegido por un mutex para acceder de forma segura a la responseQueue
    {
        std::lock_guard<std::mutex> lock(responseMutex); // Adquiere un lock del mutex al entrar al bloque, se libera automáticamente al salir
        responseQueue.push(msg);                      // Agrega el mensaje recibido a la cola de respuestas
    }
    responseCV.notify_one(); // Notifica a un thread que esté esperando en la condition variable que hay un nuevo mensaje
}

// Método para manejar las respuestas recibidas del Interconnect
void PE::handleResponses() {
    std::unique_lock<std::mutex> lock(responseMutex); // Adquiere un unique lock del mutex, permite esperas condicionales

    // Mientras la cola de respuestas no esté vacía
    while (!responseQueue.empty()) {
        if (stepByStep) {
            {
                std::lock_guard<std::mutex> lock(cin_mutex);
                std::cin.get(); // Espera a que el usuario presione Enter
            }
        }

        Message msg = responseQueue.front(); // Obtiene el mensaje del frente de la cola
        responseQueue.pop();                 // Remueve el mensaje del frente de la cola
        lock.unlock();                       // Libera el lock para permitir que otros threads (ej: el que llama a receiveResponse) accedan a la cola

        // Si el tipo de mensaje es READ_RESP (respuesta a una lectura de memoria)
        if (msg.type == MessageType::READ_RESP) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Recibido READ_RESP Actualizado Linea Caché Addr 0x"
                          << std::hex << msg.addr << "\n";
            }
            writeToCache(msg.addr, msg.data); // Escribe los datos recibidos en la caché
        }
        // Si el tipo de mensaje es WRITE_RESP (respuesta a una escritura en memoria)
        else if (msg.type == MessageType::WRITE_RESP) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Recibido WRITE_RESP Escritura Confirmada\n";
            }
        }
        // Si el tipo de mensaje es INV_ACK (respuesta a una invalidación)
        else if (msg.type == MessageType::INV_ACK) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Recibido INV_ACK del PE " << int(msg.src) << "\n";
            }
        }
        // Si el tipo de mensaje es INV_COMPLETE (indicación de que todas las invalidaciones fueron completadas)
        else if (msg.type == MessageType::INV_COMPLETE) {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Recibido INV_COMPLETE. Invalidaciones completadas.\n";
            }
        }
        // Si el tipo de mensaje no es reconocido
        else {
            {
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "PE " << id << ": Recibió tipo de mensaje inesperado: " << static_cast<int>(msg.type) << "\n";
            }
        }

        lock.lock(); // Re-adquiere el lock antes de la siguiente iteración del bucle
    }
}

// Método para iniciar la ejecución del PE en un nuevo thread
void PE::start() {
    thread = std::thread(&PE::execute, this); // Crea un nuevo thread que ejecutará la función 'execute' de este objeto PE
}

// Método para esperar a que el thread del PE termine su ejecución
void PE::join() {
    if (thread.joinable()) // Verifica si el thread puede ser unido (si aún no ha terminado)
        thread.join();     // Espera a que el thread termine
}

// Método que contiene el bucle principal de ejecución del PE
void PE::execute() {
    for (const auto& instr : instructionMemory) { // Itera a través de cada instrucción cargada en la memoria de instrucciones
        if (stepByStep) {
            {
                std::lock_guard<std::mutex> lock(cin_mutex);
                std::cin.get(); // Espera a que el usuario presione Enter
            }
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "PE " << id << ": Instrucción → " << instr << "\n";
        }
        executeInstruction(instr); // Ejecuta la instrucción actual
    }
}

// Método para escribir datos en la caché del PE
void PE::writeToCache(uint32_t addr, const std::vector<uint8_t>& data) {
    size_t blockIndex = (addr / 16) % NUM_BLOCKS; // Calcula el índice del bloque de caché usando la dirección y el tamaño del bloque
    cache[blockIndex].tag = addr / 16;           // Almacena la etiqueta (parte superior de la dirección) en el bloque de caché
    // Copia los datos al bloque de caché, asegurándose de no escribir más allá del tamaño del bloque (16 bytes)
    std::copy(data.begin(), data.begin() + std::min(data.size(), size_t(16)), cache[blockIndex].data.begin());
    cache[blockIndex].valid = true;              // Marca el bloque de caché como válido
}

// Método para leer datos de la caché del PE
std::vector<uint8_t> PE::readFromCache(uint32_t addr, size_t size) {
    size_t blockIndex = (addr / 16) % NUM_BLOCKS; // Calcula el índice del bloque de caché
    // Verifica si el bloque de caché es válido y si la etiqueta coincide con la dirección solicitada
    if (cache[blockIndex].valid && cache[blockIndex].tag == addr / 16) {
        // Si hay un cache hit, devuelve un vector de bytes con los datos solicitados
        return std::vector<uint8_t>(cache[blockIndex].data.begin(), cache[blockIndex].data.begin() + size);
    } else {
        // Si hay un cache miss, devuelve un vector vacío
        return {};
    }
}

// Método getter para obtener el ID del PE
int PE::getId() const {
    return id;
}

// Método getter para obtener el valor de QoS del PE
uint8_t PE::getQoS() const {
    return qos;
}
