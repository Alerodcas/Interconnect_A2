#include <string>
#include <vector>
#include "PE.hpp"

#include "PE.hpp"

int main() {
    PE pe(0, 0x10);
    pe.loadInstructions("workloads/workload_0.txt");
    pe.start();
    pe.join();
    return 0;
}

/*
int main() {
    std::vector<PE> pes;

    // Crear 4 PE con diferentes QoS
    for (int i = 0; i < 4; ++i) {
        pes.emplace_back(i, 0x10 + i);  // IDs: 0-3, QoS: 0x10, 0x11, ...
        pes[i].loadInstructions("workload_" + std::to_string(i) + ".txt");
    }

    // Iniciar todos los hilos
    for (auto& pe : pes) pe.start();

    // Esperar a que terminen
    for (auto& pe : pes) pe.join();

    return 0;
}
*/