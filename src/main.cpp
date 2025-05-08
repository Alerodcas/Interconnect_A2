#include <iostream>
#include <PE.hpp>
#include "Interconnect.hpp"
#include <mutex>
#include <fstream>

std::mutex cout_mutex; // Declaración del mutex global para proteger std::cout
std::mutex cin_mutex; // Declaración del mutex global para proteger std::cin
int executionMode = 0; // Número para modo de ejecución (0 -> FIFO, 1 -> Prioridad)

int main(int argc, char *argv[]) {

    //  -------------------------------------------
    //  |        Comprobaciones de argumentos     |
    //  -------------------------------------------

    // Verificar si se proporcionaron argumentos suficientes
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <modo_ejecución (0|1)> <número_test (1|2)>\n";
        return 1;
    }

    // Procesar el primer argumento
    try {
        executionMode = std::stoi(argv[1]);
        if (executionMode < 0 || executionMode > 1) {
            std::cerr << "Error: Modo de ejecución inválido. Debe ser 0 (FIFO) o 1 (Prioridad).\n";
            return 1;
        }
        std::string executionString;
        if (executionMode == 0) executionString = "0) FIFO"; else executionString = "1) Prioridad";
        std::cout << "<< Modo de ejecución seleccionado: " << executionString << " >>\n";
    } catch (...) {
        std::cerr << "Error: El primer argumento debe ser un número válido (0 o 1).\n";
        return 1;
    }

    // Procesar el segundo argumento
    int testNumber;
    try {
        testNumber = std::stoi(argv[2]);
        if (testNumber != 1 && testNumber != 2) {
            std::cerr << "Error: El número de test debe ser 1 o 2.\n";
            return 1;
        }
        std::cout << "<< Ejecutando Test " << testNumber << " >>\n";
    } catch (...) {
        std::cerr << "Error: El segundo argumento debe ser un número válido (1 o 2).\n";
        return 1;
    }

    //  -------------------------------------------
    //  |          Preparar Docs de Salida        |
    //  -------------------------------------------

    std::vector<std::string> fileNames = {
        "../output/intconnect.txt"
    };
    for (int i = 0; i < 8; ++i) {
        fileNames.push_back("../output/pe" + std::to_string(i) + ".txt");
    }
    for (const auto& fileName : fileNames) {
        std::ofstream ofs(fileName, std::ios::trunc); // Abre el archivo y lo trunca (vacía)
        if (!ofs) {
            std::cerr << "Error al intentar limpiar el archivo: " << fileName << "\n";
        } else {
            ofs << "Instrucción Recibido/Enviado Tamaño Fuente/Destino Ciclo\n";
        }
    }

    //  -------------------------------------------
    //  | Inicio de la Funcionalidad del programa |
    //  -------------------------------------------

    Interconnect interconnect;
    interconnect.start();

    std::vector<std::unique_ptr<PE>> pes;
    std::string instructionPath = "../workloads/test" + std::to_string(testNumber);
    std::cout << "<< Cargando instrucciones desde: " << instructionPath << " >>\n";
    std::cout << "<< Presiona Enter para avanzar al siguiente paso >>\n";

    for (int i = 0; i < 8; i++) {
        auto pe = std::make_unique<PE>(i, 0x00 + i, &interconnect);
        interconnect.registerPE(i, pe.get());
        pes.push_back(std::move(pe));
        pes[i]->loadInstructions(instructionPath + "/workload_" + std::to_string(i) + ".txt");
    }

    for (auto& pe : pes) pe->start();
    for (auto& pe : pes) pe->join();

    interconnect.stop();

    //  -------------------------------------------
    //  |     Ejecución Script de Graficación     |
    //  -------------------------------------------

    system("python3 graph.py");

    return 0;
}

