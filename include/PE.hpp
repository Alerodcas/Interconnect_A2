#ifndef PE_HPP
#define PE_HPP

#include <string>
#include <vector>
#include <thread>

class PE {
public:
    PE(int id, uint8_t qos);

    void loadInstructions(const std::string& filepath);
    void start();
    void join();

    int getId() const;
    uint8_t getQoS() const;

private:
    void execute();  // función para el hilo
    int id;
    uint8_t qos;
    std::vector<std::string> instructionMemory;
    std::thread thread;
};

#endif // PE_HPP
