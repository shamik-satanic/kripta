set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLIENT="$SCRIPT_DIR/rc4_client"
SERVER="$SCRIPT_DIR/rc4_server"
GEN="$SCRIPT_DIR/gen_testfiles"
WORK="$SCRIPT_DIR/demo_files"
PASS=0
FAIL=0

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

header()  { echo -e "\n${BOLD}${CYAN}══════════════════════════════════════════════════${NC}"; \
            echo -e "${BOLD}${CYAN}  $*${NC}"; \
            echo -e "${BOLD}${CYAN}══════════════════════════════════════════════════${NC}"; }
section() { echo -e "\n${YELLOW}── $* ──${NC}"; }
ok()      { echo -e "  ${GREEN}✓${NC} $*"; PASS=$((PASS + 1)); }
fail()    { echo -e "  ${RED}✗${NC} $*"; FAIL=$((FAIL + 1)); }

SERVER_PID=""
cleanup() {
    echo -e "\n${YELLOW}[demo] Остановка сервера...${NC}"
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
    rm -rf "$WORK"
    echo "[demo] Готово."
}
trap cleanup EXIT

# ── Шаг 0: Проверка бинарников ───────────────────────────────────────────────
header "Шаг 0: Проверка бинарников"
for bin in "$CLIENT" "$SERVER" "$GEN"; do
    if [ ! -x "$bin" ]; then
        echo -e "  ${RED}Не найден: $bin${NC}"
        echo "  Запустите: make"
        exit 1
    fi
    echo -e "  ${GREEN}OK${NC} $bin"
done

# ── Шаг 1: Запуск сервера ─────────────────────────────────────────────────────
header "Шаг 1: Запуск RC4-сервера"

echo "  Останавливаем старые экземпляры..."
pkill -f "rc4_server$" 2>/dev/null || true
sleep 0.5
"$SERVER" --clean 2>/dev/null || true
sleep 0.2

"$SERVER" &
SERVER_PID=$!
echo "  PID сервера: $SERVER_PID"
sleep 1.5

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "  ${RED}Сервер не запустился!${NC}"; exit 1
fi
echo -e "  ${GREEN}Сервер запущен.${NC}"

# ── Шаг 2: Генерация тестовых файлов ─────────────────────────────────────────
header "Шаг 2: Создание тестовых файлов"
mkdir -p "$WORK"
"$GEN" "$WORK"

# ── Функция тестирования ──────────────────────────────────────────────────────
test_file() {
    local label="$1" input="$2" key="$3"
    local enc="$WORK/${label}_enc.bin"
    local dec="$WORK/${label}_dec.out"
    local sz
    sz=$(wc -c < "$input" 2>/dev/null || echo 0)

    echo -e "\n  ${BOLD}[$label]${NC} $(basename "$input") (${sz} байт) | ключ=\"$key\""

    local out
    if ! out=$("$CLIENT" encrypt "$key" "$input" "$enc" 2>&1); then
        fail "[$label] Ошибка шифрования: $out"; return
    fi
    echo "$out" | grep -E "Файл|Время|Результат" | sed 's/^/    /'

    if ! out=$("$CLIENT" decrypt "$key" "$enc" "$dec" 2>&1); then
        fail "[$label] Ошибка дешифрования: $out"; return
    fi
    echo "$out" | grep -E "Файл|Время|Результат" | sed 's/^/    /'

    if "$CLIENT" compare "$input" "$dec" > /dev/null 2>&1; then
        ok "[$label] round-trip: оригинал == дешифрованный ✓"
    else
        fail "[$label] round-trip ПРОВАЛЕН"
    fi

    if [ "$sz" -gt 0 ]; then
        if ! "$CLIENT" compare "$input" "$enc" > /dev/null 2>&1; then
            ok "[$label] шифртекст отличается от открытого текста ✓"
        else
            fail "[$label] шифртекст совпадает с открытым текстом!"
        fi
    fi
}

# ── Шаг 3: Тестирование файлов ────────────────────────────────────────────────
header "Шаг 3: Шифрование и дешифрование файлов"

section "Пустой файл"
test_file "empty"   "$WORK/empty.bin"      "AnyKey123"

section "Текстовый малый (226 байт) — три разных ключа"
test_file "txt_k1"  "$WORK/text_small.txt" "SimpleKey"
test_file "txt_k2"  "$WORK/text_small.txt" "Another_Key_123!"
test_file "txt_k3"  "$WORK/text_small.txt" "К"

section "Текстовый большой (100 КБ)"
test_file "txt_big" "$WORK/text_large.txt" "LongTextKey2024"

section "Бинарный (50 КБ)"
test_file "binary"  "$WORK/binary.bin"     "BinaryKey!@#"

section "Изображение BMP (100×100, ~30 КБ)"
test_file "image"   "$WORK/image.bmp"      "ImageEncKey"

section "Аудио WAV (440 Гц, 1 с, ~86 КБ)"
test_file "audio"   "$WORK/audio.wav"      "AudioKey_RC4"

section "Видео AVI (~1 МБ)"
test_file "video"   "$WORK/video.avi"      "VideoKey2024"

# ── Шаг 4: Неверный ключ ─────────────────────────────────────────────────────
header "Шаг 4: Неверный ключ не восстанавливает данные"
{
    enc="$WORK/badkey_enc.bin"
    dec="$WORK/badkey_dec.txt"
    "$CLIENT" encrypt "CorrectKey" "$WORK/text_small.txt" "$enc" > /dev/null 2>&1
    "$CLIENT" decrypt "WrongKey"   "$enc"                 "$dec" > /dev/null 2>&1
    if ! "$CLIENT" compare "$WORK/text_small.txt" "$dec" > /dev/null 2>&1; then
        ok "Неверный ключ НЕ восстанавливает данные ✓"
    else
        fail "Неверный ключ случайно восстановил данные"
    fi
}

# ── Шаг 5: Параллельная обработка ────────────────────────────────────────────
header "Шаг 5: Параллельная обработка (20 клиентов одновременно)"
PARALLEL_N=20
par_pids=()
for i in $(seq 1 $PARALLEL_N); do
    (
        enc="$WORK/par_${i}_enc.bin"
        dec="$WORK/par_${i}_dec.out"
        "$CLIENT" encrypt "ParallelKey_${i}" "$WORK/text_small.txt" "$enc" > /dev/null 2>&1
        "$CLIENT" decrypt "ParallelKey_${i}" "$enc" "$dec"           > /dev/null 2>&1
    ) &
    par_pids+=($!)
done
for pid in "${par_pids[@]}"; do wait "$pid" 2>/dev/null || true; done

par_ok=0; par_fail=0
for i in $(seq 1 $PARALLEL_N); do
    if "$CLIENT" compare "$WORK/text_small.txt" \
                         "$WORK/par_${i}_dec.out" > /dev/null 2>&1; then
        par_ok=$((par_ok + 1))
    else
        par_fail=$((par_fail + 1))
    fi
done
if [ "$par_fail" -eq 0 ]; then
    ok "Все $PARALLEL_N параллельных сессий успешно ($par_ok/$PARALLEL_N) ✓"
else
    fail "$par_fail из $PARALLEL_N сессий провалились"
fi

# ── Шаг 6: Функция compare ───────────────────────────────────────────────────
header "Шаг 6: Функция сравнения файлов"

"$CLIENT" compare "$WORK/text_small.txt" "$WORK/text_small.txt" && \
    ok "compare(file, file) → идентичны ✓" || \
    fail "compare(file, file) → различны?!"

if ! "$CLIENT" compare "$WORK/text_small.txt" \
                        "$WORK/text_large.txt" > /dev/null 2>&1; then
    ok "compare(small, large) → различны ✓"
else
    fail "compare(small, large) → идентичны?!"
fi

touch "$WORK/empty2.bin"
"$CLIENT" compare "$WORK/empty.bin" "$WORK/empty2.bin" && \
    ok "compare(empty, empty) → идентичны ✓" || \
    fail "compare(empty, empty) → различны?!"

# ── Итоги ─────────────────────────────────────────────────────────────────────
header "Итоги демонстрации"
echo -e "  ${GREEN}Успешно : $PASS${NC}"
if [ "$FAIL" -gt 0 ]; then
    echo -e "  ${RED}Провалено: $FAIL${NC}"
    echo -e "\n  ${BOLD}${RED}Некоторые тесты провалились.${NC}"
else
    echo -e "  ${GREEN}Провалено: 0${NC}"
    echo -e "\n  ${BOLD}${GREEN}Все тесты прошли успешно!${NC}"
fi
exit "$FAIL"