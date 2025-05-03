#include <iostream>
#include <PE.hpp>
#include "Interconnect.hpp"
#include <mutex>

std::mutex cout_mutex; // Declaración del mutex global para proteger std::cout
std::mutex cin_mutex; // Declaración del mutex global para proteger std::cout
bool stepByStep = false;

int main() {
    Interconnect interconnect;
    interconnect.start();

    std::vector<std::unique_ptr<PE>> pes;

    for (int i = 0; i < 2; i++) {
        auto pe = std::make_unique<PE>(i, 0x00 + i, &interconnect);
        interconnect.registerPE(i, pe.get());
        pes.push_back(std::move(pe));
        pes[i]->loadInstructions("../workloads/workload_" + std::to_string(i) + ".txt");
    }

    if (stepByStep) {
        std::cout << "<<< Ejecución paso a paso habilitada: Presiona Enter para siguiente paso >>>\n";
    }

    for (auto& pe : pes) pe->start();
    for (auto& pe : pes) pe->join();

    /*
    // Ejecuta el script de Python para graficar
    int status = system("python graph.py");

    if (status == -1) {
        printf("Error al ejecutar el script de Python.\n");
    } else {
        printf("Script de Python ejecutado correctamente.\n");
    }
    */

    interconnect.stop();
    return 0;
}

