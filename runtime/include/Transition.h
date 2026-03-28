#pragma once
class Transition {
public:
    Transition() = default;
    Transition(const char* name, int duration, int color, int flags) : Name(name), Duration(duration), Color(color), Flags(flags) {}
    ~Transition() = default;
    void Update(float ms) {
        if (isProcessing) Elapsed += 1.0f * ms;
        if (Elapsed >= (Duration / 1000.0f)) {
            isProcessing = false;
            hasTransitioned = true;
        }
    }
    const char* Name;
    int Duration = 0;
    float Elapsed = 0.0f;

    int Color = 0;
    int Flags = 0;
    bool isProcessing = false;
    bool hasTransitioned = false;
};