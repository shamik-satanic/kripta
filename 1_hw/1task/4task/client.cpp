// ─────────────────────────────────────────────────────────────────────────────
// client.cpp — RC4-клиент.
//
// Использование:
//   ./rc4_client encrypt <key> <input_file> <output_file>
//   ./rc4_client decrypt <key> <input_file> <output_file>
//   ./rc4_client compare <file1> <file2>
//
// Клиент подключается к серверу через разделяемую память и семафоры,
// передаёт ключ и данные, получает результат шифрования/дешифрования.
// ─────────────────────────────────────────────────────────────────────────────

#include "protocol.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <chrono>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

static std::vector<uint8_t> read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open())
        throw std::runtime_error("Не удалось открыть файл: " + path);
    std::streamsize size = f.tellg();
    if (size < 0)
        throw std::runtime_error("Ошибка определения размера файла: " + path);
    f.seekg(0);
    std::vector<uint8_t> buf(static_cast<std::size_t>(size));
    if (size > 0)
        f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

static void write_file(const std::string& path,
                        const uint8_t* data, std::size_t len)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f.is_open())
        throw std::runtime_error("Не удалось создать файл: " + path);
    if (len > 0)
        f.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(len));
}

static bool compare_files(const std::string& path1, const std::string& path2) {
    auto a = read_file(path1);
    auto b = read_file(path2);
    if (a.size() != b.size()) return false;
    return a == b;
}

class RC4Client {
public:
    RC4Client() {
        // Открываем разделяемую память слотов
        int slots_fd = shm_open(SHM_SLOTS_NAME, O_RDWR, 0);
        if (slots_fd == -1)
            throw std::runtime_error(
                "shm_open(slots) failed: " + std::string(strerror(errno)) +
                "\n  Сервер запущен? Запустите: ./rc4_server");

        m_slots = static_cast<SessionSlot*>(
            mmap(nullptr, SLOTS_SHM_SIZE, PROT_READ | PROT_WRITE,
                 MAP_SHARED, slots_fd, 0));
        close(slots_fd);
        if (m_slots == MAP_FAILED)
            throw std::runtime_error("mmap(slots) failed: " +
                                     std::string(strerror(errno)));

        // Открываем общий семафор свободных слотов
        m_sem_free = sem_open(SEM_FREE_NAME, 0);
        if (m_sem_free == SEM_FAILED)
            throw std::runtime_error("sem_open(free) failed: " +
                                     std::string(strerror(errno)));
    }

    ~RC4Client() {
        if (m_slots && m_slots != MAP_FAILED)
            munmap(m_slots, SLOTS_SHM_SIZE);
        if (m_sem_free && m_sem_free != SEM_FAILED)
            sem_close(m_sem_free);
        for (int i = 0; i < MAX_SESSIONS; ++i) {
            if (m_req_sem && m_req_sem != SEM_FAILED) sem_close(m_req_sem);
            if (m_rsp_sem && m_rsp_sem != SEM_FAILED) sem_close(m_rsp_sem);
            break; // закрываем только один открытый сем (если был захвачен слот)
        }
        // Если слот был захвачен — освобождаем
        if (m_slot_idx >= 0) {
            m_slots[m_slot_idx].state = SLOT_FREE;
            sem_post(m_sem_free);
        }
    }

    std::vector<uint8_t> process(const std::string& key,
                                  const std::vector<uint8_t>& input,
                                  int timeout_sec = 30)
    {
        if (key.empty())
            throw std::invalid_argument("Ключ не может быть пустым");
        if (key.size() > MAX_KEY_LEN)
            throw std::invalid_argument("Ключ слишком длинный (макс. " +
                                        std::to_string(MAX_KEY_LEN) + " байт)");

        // ── Шаг 1: Ожидаем свободный слот ────────────────────────────────
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_sec;

        if (sem_timedwait(m_sem_free, &ts) != 0) {
            throw std::runtime_error(
                "Таймаут ожидания свободного слота: все " +
                std::to_string(MAX_SESSIONS) + " сессий заняты");
        }

        // ── Шаг 2: Захватываем свободный слот ────────────────────────────
        m_slot_idx = -1;
        for (int i = 0; i < MAX_SESSIONS; ++i) {
            SlotState expected = SLOT_FREE;
            // Атомарная CAS-замена через __atomic (C11/POSIX, без C++11 atomic на volatile)
            if (__sync_bool_compare_and_swap(
                    reinterpret_cast<volatile int32_t*>(&m_slots[i].state),
                    static_cast<int32_t>(SLOT_FREE),
                    static_cast<int32_t>(SLOT_BUSY))) {
                m_slot_idx = i;
                break;
            }
            (void)expected;
        }
        if (m_slot_idx < 0) {
            sem_post(m_sem_free);
            throw std::runtime_error("Не удалось захватить слот (internal error)");
        }

        // ── Шаг 3: Открываем семафоры слота ──────────────────────────────
        m_req_sem = sem_open(sem_req_name(m_slot_idx).c_str(), 0);
        if (m_req_sem == SEM_FAILED)
            throw std::runtime_error("sem_open(req) failed: " +
                                     std::string(strerror(errno)));
        m_rsp_sem = sem_open(sem_rsp_name(m_slot_idx).c_str(), 0);
        if (m_rsp_sem == SEM_FAILED)
            throw std::runtime_error("sem_open(rsp) failed: " +
                                     std::string(strerror(errno)));

        // ── Шаг 4: Записываем ключ в слот ────────────────────────────────
        SessionSlot& slot = m_slots[m_slot_idx];
        slot.key_len  = static_cast<uint32_t>(key.size());
        slot.data_len = input.size();
        slot.client_pid = static_cast<uint32_t>(getpid());
        memcpy(slot.key, key.data(), key.size());
        memset(slot.error_msg, 0, sizeof(slot.error_msg));

        // ── Шаг 5: Создаём разделяемый буфер для данных ──────────────────
        const std::string data_shm = shm_data_name(m_slot_idx);
        shm_unlink(data_shm.c_str()); // на случай остатка

        std::size_t buf_size = input.empty() ? 1u : input.size();
        int data_fd = shm_open(data_shm.c_str(),
                               O_CREAT | O_RDWR | O_TRUNC, 0600);
        if (data_fd == -1)
            throw std::runtime_error("shm_open(data) failed: " +
                                     std::string(strerror(errno)));

        if (ftruncate(data_fd, static_cast<off_t>(buf_size)) != 0) {
            close(data_fd);
            throw std::runtime_error("ftruncate(data) failed: " +
                                     std::string(strerror(errno)));
        }

        void* data_ptr = mmap(nullptr, buf_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED, data_fd, 0);
        close(data_fd);
        if (data_ptr == MAP_FAILED)
            throw std::runtime_error("mmap(data) failed: " +
                                     std::string(strerror(errno)));

        // Копируем данные в разделяемую память
        if (!input.empty())
            memcpy(data_ptr, input.data(), input.size());

        // ── Шаг 6: Уведомляем сервер о готовности запроса ─────────────────
        slot.state = SLOT_READY;
        sem_post(m_req_sem);

        // ── Шаг 7: Ожидаем ответа от сервера ─────────────────────────────
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += timeout_sec;

        if (sem_timedwait(m_rsp_sem, &ts) != 0) {
            munmap(data_ptr, buf_size);
            shm_unlink(data_shm.c_str());
            slot.state = SLOT_FREE;
            m_slot_idx = -1;
            sem_post(m_sem_free);
            throw std::runtime_error("Таймаут ожидания ответа сервера");
        }

        // ── Шаг 8: Проверяем статус и читаем результат ───────────────────
        if (slot.state == SLOT_ERROR) {
            std::string err = slot.error_msg;
            munmap(data_ptr, buf_size);
            shm_unlink(data_shm.c_str());
            slot.state = SLOT_FREE;
            m_slot_idx = -1;
            sem_post(m_sem_free);
            throw std::runtime_error("Сервер вернул ошибку: " + err);
        }

        // Читаем зашифрованные данные
        std::vector<uint8_t> result;
        if (!input.empty()) {
            result.assign(reinterpret_cast<uint8_t*>(data_ptr),
                          reinterpret_cast<uint8_t*>(data_ptr) + input.size());
        }

        // ── Шаг 9: Освобождаем ресурсы ───────────────────────────────────
        munmap(data_ptr, buf_size);
        shm_unlink(data_shm.c_str());
        slot.state = SLOT_FREE;
        m_slot_idx = -1;          // предотвращаем повторное освобождение в деструкторе
        sem_post(m_sem_free);     // возвращаем слот в пул

        sem_close(m_req_sem); m_req_sem = nullptr;
        sem_close(m_rsp_sem); m_rsp_sem = nullptr;

        return result;
    }

private:
    SessionSlot* m_slots    = nullptr;
    sem_t*       m_sem_free = nullptr;
    sem_t*       m_req_sem  = nullptr;
    sem_t*       m_rsp_sem  = nullptr;
    int          m_slot_idx = -1;
};

static std::string fmt_size(std::size_t n) {
    if (n == 0)          return "0 байт (пустой)";
    if (n < 1024)        return std::to_string(n) + " байт";
    if (n < 1024*1024)   return std::to_string(n/1024) + " КБ";
    if (n < 1024*1024*1024ull)
                         return std::to_string(n/(1024*1024)) + " МБ";
    return std::to_string(n/(1024*1024*1024ull)) + " ГБ";
}

static void print_usage(const char* prog) {
    std::cerr
        << "Использование:\n"
        << "  " << prog << " encrypt <key> <input_file> <output_file>\n"
        << "  " << prog << " decrypt <key> <input_file> <output_file>\n"
        << "  " << prog << " compare <file1> <file2>\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string cmd = argv[1];

    if (cmd == "compare") {
        if (argc != 4) {
            std::cerr << "Ошибка: compare требует два файла.\n";
            print_usage(argv[0]);
            return 1;
        }
        const std::string f1 = argv[2];
        const std::string f2 = argv[3];
        try {
            bool equal = compare_files(f1, f2);
            if (equal) {
                std::cout << "✓ Файлы ИДЕНТИЧНЫ: " << f1 << " == " << f2 << "\n";
                return 0;
            } else {
                auto a = read_file(f1);
                auto b = read_file(f2);
                std::cout << "✗ Файлы РАЗЛИЧАЮТСЯ: "
                          << f1 << " (" << fmt_size(a.size()) << ")"
                          << " != "
                          << f2 << " (" << fmt_size(b.size()) << ")\n";
                return 1;
            }
        } catch (const std::exception& ex) {
            std::cerr << "Ошибка: " << ex.what() << "\n";
            return 2;
        }
    }

    if (cmd == "encrypt" || cmd == "decrypt") {
        if (argc != 5) {
            std::cerr << "Ошибка: " << cmd << " требует 3 аргумента.\n";
            print_usage(argv[0]);
            return 1;
        }
        const std::string key        = argv[2];
        const std::string input_path = argv[3];
        const std::string output_path= argv[4];

        try {
            // Читаем входной файл
            auto input = read_file(input_path);
            std::cout << "  Файл    : " << input_path
                      << " (" << fmt_size(input.size()) << ")\n";
            std::cout << "  Ключ    : \"" << key << "\"\n";
            std::cout << "  Операция: " << cmd << "\n";

            // Подключаемся к серверу и выполняем операцию
            RC4Client client;
            auto t0 = std::chrono::steady_clock::now();
            auto result = client.process(key, input);
            auto t1 = std::chrono::steady_clock::now();

            double ms = std::chrono::duration<double, std::milli>(t1-t0).count();

            // Записываем результат
            write_file(output_path, result.data(), result.size());

            std::cout << "  Результат: " << output_path
                      << " (" << fmt_size(result.size()) << ")\n";
            std::cout << "  Время   : " << ms << " мс\n";
            return 0;

        } catch (const std::exception& ex) {
            std::cerr << "Ошибка: " << ex.what() << "\n";
            return 2;
        }
    }

    std::cerr << "Неизвестная команда: " << cmd << "\n";
    print_usage(argv[0]);
    return 1;
}