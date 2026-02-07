#pragma once
#include <cstdint>
#include <stdint.h>
#include <string>
#include <vector>

class Font {
  public:
    struct Glyph {
        char character;
        uint8_t width;
        uint8_t height;
        uint8_t x_offset;
        const uint8_t *data;
    };

    static const uint8_t FONT_DATA[];
    static const Glyph FONT_TABLE[];
    static const size_t FONT_TABLE_SIZE;

    Font() = default;

    const Glyph *getGlyph(char c) const;
    uint8_t getHeight() const { return 7; } // Standard height
    uint8_t getCharWidth(char c) const;
    uint8_t getCharSpacing(char c) const; // Get spacing after character};
};
extern Font defaultFont;
