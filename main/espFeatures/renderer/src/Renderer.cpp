#include "Renderer.hpp"
#include "jac/device/logger.h"
#include <algorithm>

Renderer::Renderer(int width, int height, const Color &backgroundColor)
    : width(width), height(height), backgroundColor(backgroundColor) {}

void Renderer::render(Pixels &pixels,
                      const std::vector<Collection *> &collections,
                      const DrawOptions &options) {
    std::vector<Collection *> sortedCollections = collections;
    std::sort(
        sortedCollections.begin(), sortedCollections.end(),
        [](Collection *a, Collection *b) { return a->getZ() < b->getZ(); });

    for (Collection *collection : sortedCollections) {
        collection->draw(pixels, options);
    }
}

void Renderer::drawText(Pixels &pixels, const std::string &text, int x, int y,
                        const Font &font, const Color &color, bool wrap) {
    int currentX = x;
    int currentY = y;
    int lineHeight = font.getHeight() + 1;

    for (char c : text) {
        if (c == '\n') {
            currentX = x;
            currentY += lineHeight;
            continue;
        }

        const Font::Glyph *glyph = font.getGlyph(c);

        if (wrap && currentX > x) {
            int charWidth = glyph ? (glyph->width + font.getCharSpacing(c)) : 5;
            if (currentX + charWidth > width) {
                currentX = x;
                currentY += lineHeight;
            }
        }

        if (!glyph) {
            for (int i = 0; i < 5; ++i) {
                pixels.push_back({currentX + i, currentY, color});
            }
            for (int i = 0; i < 5; ++i) {
                pixels.push_back({currentX + i, currentY + 6, color});
            }
            for (int i = 0; i < 7; ++i) {
                pixels.push_back({currentX, currentY + i, color});
            }
            for (int i = 0; i < 7; ++i) {
                pixels.push_back({currentX + 4, currentY + i, color});
            }
            currentX += 5;
            continue;
        }

        for (int row = 0; row < glyph->height; ++row) {
            uint8_t line = glyph->data[row];

            for (int col = 0; col < glyph->width; ++col) {
                int bit_pos = 4 - (col + glyph->x_offset);
                if (bit_pos >= 0 && (line & (1 << bit_pos))) {
                    pixels.push_back({currentX + col, currentY + row, color});
                }
            }
        }

        currentX += glyph->width + font.getCharSpacing(c);
    }
}
