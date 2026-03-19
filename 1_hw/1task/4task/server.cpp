#include "protocol.hpp"
#include "rc4.hpp"

#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdio>
#include <csignal>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

// ─────────────────────────────────────────────────────────────────────────────
// Глобальные ресурсы (для корректного освобождения при завершении)
// ─────────────────────────────────────────────────────────────────────────────

static SessionSlot* g_slots    = nullptr;
static sem_t*       g_sem_free = nullptr;
static sem_t*       g_req_sems[MAX_SESSIONS];
static sem_t*       g_rsp_sems[MAX_SESSIONS];
static int          g_data_fds [MAX_SESSIONS];

static std::atomic<bool> g_running{true};

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательные функции
// ─────────────────────────────────────────────────────────────────────────────

static void cleanup_ipc() {
    std::cout << "\n[server] Освобождение IPC-ресурсов...\n";

    // Слоты: разрегистрация + удаление shm
    if (g_slots != nullptr && g_slots != MAP_FAILED) {
        munmap(g_slots, SLOTS_SHM_SIZE);
    }
    shm_unlink(SHM_SLOTS_NAME);

    // Общий семафор свободных слотов
    if (g_sem_free && g_sem_free != SEM_FAILED) {
        sem_close(g_sem_free);
    }
    sem_unlink(SEM_FREE_NAME);

    // Слот-специфичные ресурсы
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        if (g_req_sems[i] && g_req_sems[i] != SEM_FAILED) {
            sem_close(g_req_sems[i]);
            sem_unlink(sem_req_name(i).c_str());
        }
        if (g_rsp_sems[i] && g_rsp_sems[i] != SEM_FAILED) {
            sem_close(g_rsp_sems[i]);
            sem_unlink(sem_rsp_name(i).c_str());
        }
        shm_unlink(shm_data_name(i).c_str());
    }
    std::cout << "[server] IPC-ресурсы освобождены.\n";
}

static void signal_handler(int /*sig*/) {
    g_running.store(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Рабочий поток для одного слота
// ─────────────────────────────────────────────────────────────────────────────

static void slot_worker(int slot_idx) {
    sem_t* req_sem = g_req_sems[slot_idx];
    sem_t* rsp_sem = g_rsp_sems[slot_idx];
    SessionSlot& slot = g_slots[slot_idx];
    const std::string data_shm_name = shm_data_name(slot_idx);

    while (g_running.load()) {
        // Ждём запроса от клиента (с таймаутом для проверки g_running)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; // 1 секунда тайм-аута

        int ret = sem_timedwait(req_sem, &ts);
        if (ret != 0) {
            // ETIMEDOUT — нормально, проверяем g_running
            continue;
        }

        // Получили запрос
        const std::size_t data_len = slot.data_len;
        const uint32_t    key_len  = slot.key_len;

        // Формируем ключ
        std::vector<uint8_t> key(
            reinterpret_cast<const uint8_t*>(slot.key),
            reinterpret_cast<const uint8_t*>(slot.key) + key_len);

        // Открываем разделяемый буфер данных
        int fd = shm_open(data_shm_name.c_str(), O_RDWR, 0600);
        if (fd == -1) {
            std::snprintf(slot.error_msg, sizeof(slot.error_msg),
                          "server: shm_open failed: %s", strerror(errno));
            slot.state = SLOT_ERROR;
            sem_post(rsp_sem);
            continue;
        }

        // Маппируем буфер
        void* ptr = nullptr;
        if (data_len > 0) {
            ptr = mmap(nullptr, data_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        }
        close(fd);

        if (data_len > 0 && (ptr == MAP_FAILED || ptr == nullptr)) {
            std::snprintf(slot.error_msg, sizeof(slot.error_msg),
                          "server: mmap failed: %s", strerror(errno));
            slot.state = SLOT_ERROR;
            sem_post(rsp_sem);
            continue;
        }

        // Выполняем RC4 in-place
        try {
            if (data_len > 0) {
                auto S = rc4::detail::ksa(key);
                rc4::detail::prga(S, reinterpret_cast<uint8_t*>(ptr), data_len);
                munmap(ptr, data_len);
            }
            slot.state = SLOT_DONE;
        } catch (const std::exception& ex) {
            if (data_len > 0) munmap(ptr, data_len);
            std::snprintf(slot.error_msg, sizeof(slot.error_msg),
                          "server: RC4 error: %s", ex.what());
            slot.state = SLOT_ERROR;
        }

        // Уведомляем клиента
        sem_post(rsp_sem);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // Режим очистки IPC-объектов
    if (argc > 1 && std::string(argv[1]) == "--clean") {
        std::cout << "[server] Очистка IPC-объектов...\n";
        shm_unlink(SHM_SLOTS_NAME);
        sem_unlink(SEM_FREE_NAME);
        for (int i = 0; i < MAX_SESSIONS; ++i) {
            sem_unlink(sem_req_name(i).c_str());
            sem_unlink(sem_rsp_name(i).c_str());
            shm_unlink(shm_data_name(i).c_str());
        }
        std::cout << "[server] Готово.\n";
        return 0;
    }

    std::cout << "══════════════════════════════════════════════════\n";
    std::cout << "  RC4 Encryption Server  (MAX_SESSIONS=" << MAX_SESSIONS << ")\n";
    std::cout << "══════════════════════════════════════════════════\n";

    // Регистрируем обработчики сигналов
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    // Инициализируем указатели
    memset(g_req_sems, 0, sizeof(g_req_sems));
    memset(g_rsp_sems, 0, sizeof(g_rsp_sems));
    memset(g_data_fds, -1, sizeof(g_data_fds));

    // ── 1. Создаём разделяемую память для массива слотов ──────────────────
    int slots_fd = shm_open(SHM_SLOTS_NAME, O_CREAT | O_RDWR | O_TRUNC, 0600);
    if (slots_fd == -1) {
        std::perror("shm_open(slots)");
        return 1;
    }
    if (ftruncate(slots_fd, static_cast<off_t>(SLOTS_SHM_SIZE)) != 0) {
        std::perror("ftruncate(slots)");
        close(slots_fd);
        return 1;
    }
    g_slots = static_cast<SessionSlot*>(
        mmap(nullptr, SLOTS_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, slots_fd, 0));
    close(slots_fd);
    if (g_slots == MAP_FAILED) {
        std::perror("mmap(slots)");
        return 1;
    }
    // Инициализируем все слоты
    memset(g_slots, 0, SLOTS_SHM_SIZE);
    for (int i = 0; i < MAX_SESSIONS; ++i)
        g_slots[i].state = SLOT_FREE;

    std::cout << "[server] Разделяемая память слотов создана (" << SLOTS_SHM_SIZE << " байт)\n";

    // ── 2. Создаём общий семафор свободных слотов ─────────────────────────
    sem_unlink(SEM_FREE_NAME); // на случай остатка от предыдущего запуска
    g_sem_free = sem_open(SEM_FREE_NAME, O_CREAT | O_EXCL, 0600, MAX_SESSIONS);
    if (g_sem_free == SEM_FAILED) {
        std::perror("sem_open(free)");
        cleanup_ipc();
        return 1;
    }
    std::cout << "[server] Семафор свободных слотов создан (значение=" << MAX_SESSIONS << ")\n";

    // ── 3. Создаём семафоры для каждого слота ─────────────────────────────
    for (int i = 0; i < MAX_SESSIONS; ++i) {
        // Удаляем остатки
        sem_unlink(sem_req_name(i).c_str());
        sem_unlink(sem_rsp_name(i).c_str());

        g_req_sems[i] = sem_open(sem_req_name(i).c_str(),
                                  O_CREAT | O_EXCL, 0600, 0);
        if (g_req_sems[i] == SEM_FAILED) {
            std::perror(("sem_open req " + std::to_string(i)).c_str());
            cleanup_ipc();
            return 1;
        }
        g_rsp_sems[i] = sem_open(sem_rsp_name(i).c_str(),
                                  O_CREAT | O_EXCL, 0600, 0);
        if (g_rsp_sems[i] == SEM_FAILED) {
            std::perror(("sem_open rsp " + std::to_string(i)).c_str());
            cleanup_ipc();
            return 1;
        }
    }
    std::cout << "[server] Семафоры слотов созданы (" << MAX_SESSIONS * 2 << " штук)\n";

    // ── 4. Запускаем пул рабочих потоков ─────────────────────────────────
    std::vector<std::thread> workers;
    workers.reserve(MAX_SESSIONS);
    for (int i = 0; i < MAX_SESSIONS; ++i)
        workers.emplace_back(slot_worker, i);

    std::cout << "[server] Запущено " << MAX_SESSIONS << " рабочих потоков.\n";
    std::cout << "[server] Готов к приёму запросов. Для остановки: Ctrl+C\n";
    std::cout << "──────────────────────────────────────────────────\n";

    // ── 5. Основной цикл: просто ждём сигнала завершения ─────────────────
    while (g_running.load()) {
        sleep(1);
    }

    std::cout << "\n[server] Получен сигнал завершения. Останавливаем потоки...\n";
    g_running.store(false);

    // Будим все ожидающие потоки фиктивными постами,
    // чтобы они вышли из sem_timedwait и проверили g_running
    for (int i = 0; i < MAX_SESSIONS; ++i)
        sem_post(g_req_sems[i]);

    for (auto& t : workers)
        if (t.joinable()) t.join();

    cleanup_ipc();
    std::cout << "[server] Сервер остановлен.\n";
    return 0;
}