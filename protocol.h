#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>

enum CommandType : uint32_t {
    CMD_SEND_CONFIG = 1,
    CMD_SEND_DATA_A = 2,
    CMD_SEND_DATA_B = 3,
    CMD_START_TASK  = 4,
    CMD_GET_STATUS  = 5,
    CMD_GET_RESULT  = 6
};


enum StatusType : uint32_t {
    STATUS_OK = 100,
    STATUS_ERROR = 101,
    STATUS_IN_PROGRESS = 102,
    STATUS_DONE = 103
};

struct MessageHeader
{
    uint32_t command;
    uint32_t data_length;

};

struct ConfigPayload {
    uint32_t matrix_size;
    uint32_t num_threads;
};

#endif // PROTOCOL_H