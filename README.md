# voicectl

## Сборка

Ошибка вида `undefined reference to vosk_*` означает, что на этапе линковки не подключена (или не найдена) бинарная библиотека Vosk `libvosk.so`.

### 1) Получите `libvosk.so`

В этом репозитории лежат исходники/заголовки Vosk API в `include/vosk-api`, но готового `libvosk.so` здесь нет.

Варианты:

- Скачайте готовую `libvosk.so` из релизов `alphacep/vosk-api` и положите её в удобное место (часто кладут прямо в `include/vosk-api/src`).
- Либо соберите `libvosk.so` из `include/vosk-api/src` через `make`, но для этого нужен собранный Kaldi (см. официальные инструкции Vosk).

### 2) Соберите `voicectl` через CMake

Если `libvosk.so` лежит, например, в `include/vosk-api/src/libvosk.so`:

```bash
cmake -S . -B build -DVOSK_LIBRARY="$PWD/include/vosk-api/src/libvosk.so"
cmake --build build
```

### 2a) (опционально) Собрать `libvosk.so` из исходников через CMake

В `CMakeLists.txt` есть режим `-DVOSK_BUILD=ON`, который собирает `libvosk.so` из `include/vosk-api/src`.
Важно: для этого уже должен быть собран Kaldi (и OpenFST/BLAS), и нужно указать хотя бы `KALDI_ROOT`:

```bash
cmake -S . -B build -DVOSK_BUILD=ON -DKALDI_ROOT=/path/to/kaldi
cmake --build build
```

### 3) Запуск

Модель в репозитории находится в `include/vosk-model-small-ru-0.22`, так что запускать удобнее из корня репозитория или поправить путь в `src/main.cpp`.
