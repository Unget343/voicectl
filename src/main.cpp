#include <iostream>
#include <string>
#include <cstdio>

#include "vosk_api.h"
#include <portaudio.h>
using namespace std;

#define SAMPLE_RATE 16000
#define FRAMES 4000

int main() {

    // Загружаем модель
    VoskModel *model =
        vosk_model_new("vosk-model-small-ru-0.22");

    if (!model) {
        cout << "Failed to load model\n";
        return 1;
    }

    // Создаём recognizer
    VoskRecognizer *recognizer =
        vosk_recognizer_new(model, SAMPLE_RATE);

    // Инициализация PortAudio
    Pa_Initialize();

    PaStream *stream;

    Pa_OpenDefaultStream(
        &stream,
        1,          // input channels
        0,          // output channels
        paInt16,
        SAMPLE_RATE,
        FRAMES,
        NULL,
        NULL
    );

    Pa_StartStream(stream);

    cout << "Speak...\n";

    short buffer[FRAMES];

    while (true) 
    {
        // Читаем микрофон
        Pa_ReadStream(stream, buffer, FRAMES);

        // Отправляем звук в Vosk
        int accepted = vosk_recognizer_accept_waveform(
            recognizer,
            (const char*)buffer,
            sizeof(buffer)
        );

        // Если фраза завершена
        if (accepted) 
        {
            const char *result =
                vosk_recognizer_result(recognizer);

            string text(result);

            cout << text << endl;
           

            if (text.find("скачай обнову") != string::npos) 
            {
                system("sudo apt update");
            } else if (text.find("чекни пакеты") != string::npos) {
                system("apt list");
            } else if (text.find("какие есть модули") != string::npos)
            {
                system("sudo lsmod");
            } else if (text.find("собрать") != string::npos)
            {
                system("cmake --build .");
            } else if (text.find("что с пингом") != string::npos)
            {
                system("ping -s 0 google.com");
            }
        }
        else 
        {
            // Частичный результат
            const char *partial =
                vosk_recognizer_partial_result(recognizer);

            cout << partial << "\r";
        }
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    vosk_recognizer_free(recognizer);
    vosk_model_free(model);

    return 0;
}
