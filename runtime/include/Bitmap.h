#pragma once

#include <vector>
#include <cstring>
#include <cstdint>

class Bitmap {
public:
    Bitmap() : width(0), height(0) {}

    Bitmap(int width, int height)
    {
        Init(width, height);
    }

    void Resize(int width, int height) {
        Init(width, height);
    }

    void Clear(int color) {
        for (int i = 0; i < data.size(); i++) {
            data[i] = color;
        }
    }

    void SetPixel(int x, int y, int color) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        data[y * width + x] = color;
    }

    void DrawLine(int x1, int y1, int x2, int y2, int color) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        int steps = std::max(abs(dx), abs(dy));
        float xIncrement = dx / (float)steps;
        float yIncrement = dy / (float)steps;
        float x = (float)x1;
        float y = (float)y1;
        for (int i = 0; i <= steps; i++) {
            SetPixel((int)x, (int)y, color);
            x += xIncrement;
            y += yIncrement;
        }
    }

    void DrawRectangle(int x, int y, int width, int height, int color) {
        for (int i = x; i < x + width; i++) {
            for (int j = y; j < y + height; j++) {
                SetPixel(i, j, color);
            }
        }
    }

    void DrawRectangleLines(int x, int y, int width, int height, int color) {
        DrawLine(x, y, x + width - 1, y, color);
        DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
        DrawLine(x + width - 1, y + height - 1, x, y + height - 1, color);
        DrawLine(x, y + height - 1, x, y, color);
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    uint32_t* GetData() { return data.data(); }
private:
    std::vector<uint32_t> data;
    int width;
    int height;

    void Init(int width, int height) {
        if (this->width == width && this->height == height) return;

        this->width = width;
        this->height = height;
        data.resize(width * height);
        Clear(0);
    }
};