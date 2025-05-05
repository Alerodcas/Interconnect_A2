#include <iostream>
#include <PE.hpp>
#include "Interconnect.hpp"
#include <mutex>

std::mutex cout_mutex; // Declaración del mutex global para proteger std::cout
std::mutex cin_mutex; // Declaración del mutex global para proteger std::cin
int executionMode = 0; // Número para modo de ejecución (0 -> FIFO, 1 -> Prioridad)

int main(int argc, char *argv[]) {

    //  -------------------------------------------
    //  |        Comprobaciones de argumentos     |
    //  -------------------------------------------

    // Verificar si se proporcionaron argumentos suficientes
    if (argc == 2) {
        // Procesar el primer argumento: modo de ejecución
        try {
            executionMode = std::stoi(argv[1]);
            if (executionMode < 0 || executionMode > 1) {
                std::cerr << "Error: Modo de ejecución inválido. Debe ser 0 (FIFO) o 1 (Prioridad).\n";
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
    }

    //  -------------------------------------------
    //  | Inicio de la Funcionalidad del programa |
    //  -------------------------------------------
    Interconnect interconnect;
    interconnect.start();

    std::vector<std::unique_ptr<PE>> pes;

    for (int i = 0; i < 8; i++) {
        auto pe = std::make_unique<PE>(i, 0x00 + i, &interconnect);
        interconnect.registerPE(i, pe.get());
        pes.push_back(std::move(pe));
        pes[i]->loadInstructions("../workloads/workload_" + std::to_string(i) + ".txt");
    }

    for (auto& pe : pes) pe->start();
    for (auto& pe : pes) pe->join();

    //interconnect.stop();

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

