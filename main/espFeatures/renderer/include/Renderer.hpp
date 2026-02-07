#pragma once
#include "Font/Font.hpp"
#include "Shapes/Collection.hpp"
#include "types.h"
#include <string>
#include <vector>

class Renderer {
  private:
    int width;
    int height;
    Color backgroundColor;

  public:
    Renderer(int width, int height,
             const Color &backgroundColor = Color(0, 0, 0, 1.0f));

    void render(Pixels &pixels, const std::vector<Collection *> &collections,
                const DrawOptions &options);

    void drawText(Pixels &pixels, const std::string &text, int x, int y,
                  const Font &font, const Color &color, bool wrap = false);

    static Pixels blendPixels(const Pixels &pixels);
    static Pixel blendPixel(const Pixel &background, const Pixel &foreground);

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    const Color &getBackgroundColor() const { return backgroundColor; }
};
