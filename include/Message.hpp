#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <cstdint>
#include <vector>
#include <string>

enum class MessageType {
    READ_MEM,
    WRITE_MEM,
    BROADCAST_INVALIDATE,
    INV_ACK,
    INV_COMPLETE,
    READ_RESP,
    WRITE_RESP
};

struct Message {
    MessageType type;
    uint8_t src;                 // ID del PE origen
    uint8_t dest;                // ID del PE destino
    uint32_t addr = 0;
    uint32_t size = 0;
    std::vector<uint8_t> data;   // Para WRITE o READ_RESP
    uint8_t qos = 0x00;          // Prioridad (0x00 - 0xFF)
    bool status;                 //
};

#endif // MESSAGE_HPP
