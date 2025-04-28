#include <PE.hpp>

#include "Interconnect.hpp"

int main() {
    Interconnect interconnect;
    interconnect.start();

    std::vector<std::unique_ptr<PE>> pes;


    for (int i = 0; i < 2; i++) {
        auto pe = std::make_unique<PE>(i, 0x10 + i, &interconnect);
        interconnect.registerPE(i, pe.get());
        pes.push_back(std::move(pe));
        pes[i]->loadInstructions("../workloads/workload_" + std::to_string(i) + ".txt");
    }

    for (auto& pe : pes) pe->start();
    for (auto& pe : pes) pe->join();

    interconnect.stop();
    return 0;
}
