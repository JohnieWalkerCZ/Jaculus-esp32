#pragma once
#include "types.h"
#include <cstdint>
#include <string>
#include <vector>

class Texture {
  private:
    std::vector<std::vector<Color>> pixels;
    int width;
    int height;
    std::string wrapMode;
    bool valid;

  public:
    Texture(const std::vector<std::vector<Color>> &pixels);
    Texture();

    static bool fromBMP(const std::string &filename, Texture &outTexture,
                        bool littleEndian = true);

    Color sample(int u, int v) const;
    void setWrapMode(const std::string &mode);

    int getWidth() const { return width; }
    int getHeight() const { return height; }
    bool isValid() const { return valid; }

  private:
    static bool readFile(const std::string &filename,
                         std::vector<uint8_t> &buffer);
    static uint16_t getUint16(const uint8_t *data, size_t offset,
                              bool littleEndian = true);
    static uint32_t getUint32(const uint8_t *data, size_t offset,
                              bool littleEndian = true);
    static int32_t getInt32(const uint8_t *data, size_t offset,
                            bool littleEndian = true);
};
