#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <thread>

class Audio {
    static constexpr int RATE = 22050;

    static void play_tone(int freq1, int dur1_ms, int freq2, int dur2_ms, double volume) {
        std::thread([=]() {
            int s1 = RATE * dur1_ms / 1000;
            int s2 = RATE * dur2_ms / 1000;
            int total = s1 + s2;
            auto* buf = new int16_t[total];
            int pos = 0;

            auto gen = [&](int freq, int samples) {
                for (int i = 0; i < samples && pos < total; i++, pos++) {
                    double t = (double)i / RATE;
                    double env = 1.0;
                    int fade = samples / 5;
                    if (fade > 0) {
                        if (i < fade) env = (double)i / fade;
                        if (i > samples - fade) env = (double)(samples - i) / fade;
                    }
                    buf[pos] = (int16_t)(env * volume * 16000.0 * std::sin(2.0 * M_PI * freq * t));
                }
            };

            gen(freq1, s1);
            gen(freq2, s2);

            FILE* pipe = popen("paplay --raw --format=s16le --rate=22050 --channels=1 2>/dev/null", "w");
            if (pipe) {
                fwrite(buf, sizeof(int16_t), total, pipe);
                pclose(pipe);
            }
            delete[] buf;
        }).detach();
    }

public:
    static void beep_on()  { play_tone(600, 40, 900, 40, 0.3); }
    static void beep_off() { play_tone(900, 40, 600, 40, 0.3); }
};
