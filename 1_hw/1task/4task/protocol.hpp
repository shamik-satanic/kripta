#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

static constexpr int    MAX_SESSIONS    = 100;
static constexpr std::size_t MAX_KEY_LEN = 256;

// Имена IPC-объектов
static const char* SHM_SLOTS_NAME     = "/rc4srv_slots";
static const char* SEM_FREE_NAME      = "/rc4srv_free";

// Имена семафоров/shm для слота N формируются как base + "_" + N
static const char* SEM_REQ_BASE       = "/rc4srv_req";
static const char* SEM_RSP_BASE       = "/rc4srv_rsp";
static const char* SHM_DATA_BASE      = "/rc4srv_data";

enum SlotState : int32_t {
    SLOT_FREE  = 0,  ///< слот свободен
    SLOT_BUSY  = 1,  ///< слот занят клиентом (заполняется запрос)
    SLOT_READY = 2,  ///< запрос готов к обработке (сервер обрабатывает)
    SLOT_DONE  = 3,  ///< ответ готов, ожидает забора клиентом
    SLOT_ERROR = 4   ///< сервер вернул ошибку
};

struct alignas(64) SessionSlot {
    volatile SlotState state;            ///< состояние слота
    uint32_t           key_len;          ///< длина ключа в байтах
    uint64_t           data_len;         ///< длина данных в байтах
    char               key[MAX_KEY_LEN]; ///< ключ шифрования
    char               error_msg[256];   ///< сообщение об ошибке (если SLOT_ERROR)
    uint32_t           client_pid;       ///< PID клиента (для отладки)
    uint8_t            _pad[128];        ///< выравнивание до 512 байтов
};

static_assert(sizeof(SessionSlot) <= 1024,
              "SessionSlot is too large; check padding");

static constexpr std::size_t SLOTS_SHM_SIZE =
    sizeof(SessionSlot) * static_cast<std::size_t>(MAX_SESSIONS);

#include <string>

inline std::string sem_req_name(int slot) {
    return std::string(SEM_REQ_BASE) + "_" + std::to_string(slot);
}
inline std::string sem_rsp_name(int slot) {
    return std::string(SEM_RSP_BASE) + "_" + std::to_string(slot);
}
inline std::string shm_data_name(int slot) {
    return std::string(SHM_DATA_BASE) + "_" + std::to_string(slot);
}