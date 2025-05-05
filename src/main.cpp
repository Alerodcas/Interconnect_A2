#include <iostream>
#include <PE.hpp>
#include "Interconnect.hpp"
#include <mutex>

std::mutex cout_mutex; // Declaración del mutex global para proteger std::cout
std::mutex cin_mutex; // Declaración del mutex global para proteger std::cin
bool stepByStep = false; // Flag para tipo de ejecución
int executionMode = 0; // Número para modo de ejecución (0 -> FIFO, 1 -> Prioridad)

int main(int argc, char *argv[]) {

    //  -------------------------------------------
    //  |        Comprobaciones de argumentos     |
    //  -------------------------------------------

    // Verificar si se proporcionaron argumentos suficientes
    if (argc == 3) {
        // Procesar el primer argumento: modo de ejecución
        try {
            executionMode = std::stoi(argv[1]);
            if (executionMode < 0 || executionMode > 2) {
                std::cerr << "Error: Modo de ejecución inválido. Debe ser 1 (FIFO) o 2 (Prioridad).\n";
                return 1; // Indica un error en la ejecución
            }
            // Aquí podrías configurar el Interconnect con el modo de ejecución
            std::cout << "<< Modo de ejecución seleccionado: " << executionMode << " >>\n";
        } catch (const std::invalid_argument& e) {
            std::cerr << "Error: El primer argumento no es un número válido para el modo de ejecución.\n";
            return 1;
        } catch (const std::out_of_range& e) {
            std::cerr << "Error: El primer argumento está fuera del rango válido para el modo de ejecución.\n";
            return 1;
        }

        // Procesar el segundo argumento: step by step (true/false)
        std::string stepArg = argv[2];
        // Convertir a minúscula para una comparación sin distinción de mayúsculas
        for (char &c : stepArg) {
            c = std::tolower(c);
        }
        if (stepArg == "true") {
            stepByStep = true;
            std::cout << "<<< Ejecución paso a paso habilitada: Presiona Enter para siguiente paso >>>\n";
        } else if (stepArg == "false") {
            stepByStep = false;
            std::cout << "<< Ejecución paso a paso deshabilitada >>\n";
        } else {
            std::cerr << "Error: El segundo argumento para 'step by step' debe ser 'true' o 'false'.\n";
            return 1;
        }
    } else {
        std::cout << "<< Uso:  <modo_ejecucion> <step_by_step> (true|false)\n";
        std::cout << "Modo de ejecución: 1 (FIFO), 2 (Prioridad).\n";
        std::cout << "Ejecutando con modo FIFO por defecto y sin paso a paso.\n";
    }

    //  -------------------------------------------
    //  | Inicio de la Funcionalidad del programa |
    //  -------------------------------------------
    Interconnect interconnect;
    interconnect.start();

    std::vector<std::unique_ptr<PE>> pes;

    for (int i = 0; i < 2; i++) {
        auto pe = std::make_unique<PE>(i, 0x00 + i, &interconnect);
        interconnect.registerPE(i, pe.get());
        pes.push_back(std::move(pe));
        pes[i]->loadInstructions("../workloads/workload_" + std::to_string(i) + ".txt");
    }

    for (auto& pe : pes) pe->start();
    for (auto& pe : pes) pe->join();

    interconnect.stop();

    /*
    // Ejecuta el script de Python para graficar
    int status = system("python graph.py");

    if (status == -1) {
        printf("Error al ejecutar el script de Python.\n");
    } else {
        printf("Script de Python ejecutado correctamente.\n");
    }
    */

    return 0;
}

