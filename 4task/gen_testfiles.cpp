#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <string>

static void write_file(const std::string& path, const uint8_t* data, size_t len) {
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "Cannot create %s\n", path.c_str()); exit(1); }
    if (len > 0) fwrite(data, 1, len, f);
    fclose(f);
    printf("  Создан: %s (%zu байт)\n", path.c_str(), len);
}

static void write_u16le(uint8_t* buf, size_t off, uint16_t v) {
    buf[off]   = v & 0xFF;
    buf[off+1] = (v >> 8) & 0xFF;
}
static void write_u32le(uint8_t* buf, size_t off, uint32_t v) {
    buf[off]   =  v        & 0xFF;
    buf[off+1] = (v >>  8) & 0xFF;
    buf[off+2] = (v >> 16) & 0xFF;
    buf[off+3] = (v >> 24) & 0xFF;
}

int main(int argc, char* argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: gen_testfiles <outdir>\n"); return 1; }
    std::string dir = argv[1];
    if (dir.back() != '/') dir += '/';

    // 1. Пустой файл
    write_file(dir + "empty.bin", nullptr, 0);

    // 2. Текстовый малый
    const char* text =
        "Привет, мир! Это тест шифрования RC4.\n"
        "Hello, World! This is an RC4 encryption test.\n"
        "Данные произвольного размера успешно шифруются и дешифруются.\n";
    write_file(dir + "text_small.txt",
               reinterpret_cast<const uint8_t*>(text), strlen(text));

    // 3. Текстовый большой (100 КБ)
    {
        const size_t SZ = 100 * 1024;
        uint8_t* buf = new uint8_t[SZ];
        uint32_t lcg = 0xDEADBEEF;
        static const char CHARS[] =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789 \n.,!?";
        const int NC = (int)strlen(CHARS);
        for (size_t i = 0; i < SZ; ++i) {
            lcg = lcg * 1664525u + 1013904223u;
            buf[i] = (uint8_t)CHARS[(lcg >> 16) % NC];
        }
        write_file(dir + "text_large.txt", buf, SZ);
        delete[] buf;
    }

    // 4. Бинарный случайный (50 КБ)
    {
        const size_t SZ = 50 * 1024;
        uint8_t* buf = new uint8_t[SZ];
        uint32_t lcg = 0xCAFEBABE;
        for (size_t i = 0; i < SZ; ++i) {
            lcg = lcg * 1664525u + 1013904223u;
            buf[i] = (uint8_t)((lcg >> 8) & 0xFF);
        }
        write_file(dir + "binary.bin", buf, SZ);
        delete[] buf;
    }

    // 5. BMP 100x100 RGB (~30 КБ)
    {
        const int W = 100, H = 100;
        const int row_stride = (W * 3 + 3) & ~3;
        const int pixel_data_size = row_stride * H;
        const int file_size = 54 + pixel_data_size;
        uint8_t* buf = new uint8_t[file_size];
        memset(buf, 0, file_size);

        buf[0] = 'B'; buf[1] = 'M';
        write_u32le(buf,  2, (uint32_t)file_size);
        write_u32le(buf,  6, 0);
        write_u32le(buf, 10, 54);
        write_u32le(buf, 14, 40);
        write_u32le(buf, 18, (uint32_t)W);
        write_u32le(buf, 22, (uint32_t)H);
        write_u16le(buf, 26, 1);
        write_u16le(buf, 28, 24);
        write_u32le(buf, 30, 0);
        write_u32le(buf, 34, (uint32_t)pixel_data_size);

        uint8_t* px = buf + 54;
        uint32_t lcg = 0xBEEFCAFE;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                lcg = lcg * 1664525u + 1013904223u;
                px[y * row_stride + x * 3 + 0] = (uint8_t)(x * 255 / W);
                px[y * row_stride + x * 3 + 1] = (uint8_t)(y * 255 / H);
                px[y * row_stride + x * 3 + 2] = (uint8_t)((lcg >> 16) & 0xFF);
            }
        }
        write_file(dir + "image.bmp", buf, (size_t)file_size);
        delete[] buf;
    }

    // 6. WAV: 440 Гц, 1 с, 44100 Гц, 16 бит моно
    {
        const uint32_t sample_rate = 44100;
        const uint32_t num_samples = sample_rate;
        const uint32_t data_size   = num_samples * 2;
        const uint32_t file_size   = 44 + data_size;

        uint8_t* buf = new uint8_t[file_size];
        memset(buf, 0, file_size);

        memcpy(buf +  0, "RIFF", 4);
        write_u32le(buf,  4, file_size - 8);
        memcpy(buf +  8, "WAVE", 4);
        memcpy(buf + 12, "fmt ", 4);
        write_u32le(buf, 16, 16);
        write_u16le(buf, 20, 1);
        write_u16le(buf, 22, 1);
        write_u32le(buf, 24, sample_rate);
        write_u32le(buf, 28, sample_rate * 2);
        write_u16le(buf, 32, 2);
        write_u16le(buf, 34, 16);
        memcpy(buf + 36, "data", 4);
        write_u32le(buf, 40, data_size);

        uint8_t* samples = buf + 44;
        for (uint32_t i = 0; i < num_samples; ++i) {
            double v = sin(2.0 * 3.14159265358979 * 440.0 * i / sample_rate);
            int16_t s = (int16_t)(v * 32767.0);
            samples[i*2+0] =  s       & 0xFF;
            samples[i*2+1] = (s >> 8) & 0xFF;
        }
        write_file(dir + "audio.wav", buf, file_size);
        delete[] buf;
    }

    // 7. Псевдо-AVI (~1 МБ)
    {
        const size_t SZ = 1024 * 1024;
        uint8_t* buf = new uint8_t[SZ];
        memcpy(buf + 0, "RIFF", 4);
        write_u32le(buf, 4, (uint32_t)(SZ - 8));
        memcpy(buf + 8, "AVI ", 4);
        uint32_t lcg = 0xF00DCAFE;
        for (size_t i = 12; i < SZ; ++i) {
            lcg = lcg * 1664525u + 1013904223u;
            buf[i] = (uint8_t)((lcg >> 8) & 0xFF);
        }
        write_file(dir + "video.avi", buf, SZ);
        delete[] buf;
    }

    printf("  Все тестовые файлы созданы.\n");
    return 0;
}