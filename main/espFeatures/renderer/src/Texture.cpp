#include "Texture.hpp"
#include "esp_log.h"
#include "jac/device/logger.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

static const char *TAG = "Texture";

Texture::Texture(const std::vector<std::vector<Color>> &pixels)
    : pixels(pixels), wrapMode("repeat"), valid(true) {
    this->height = pixels.size();
    this->width = (height > 0) ? pixels[0].size() : 0;
}

Texture::Texture() : width(0), height(0), wrapMode("repeat"), valid(false) {}

bool Texture::readFile(const std::string &filename,
                       std::vector<uint8_t> &buffer) {
    fs::path filepath(filename);

    std::error_code ec;
    if (!fs::exists(filepath, ec)) {
        jac::Logger::error(
            std::format("File not found: {}", filename.c_str()).c_str());
        return false;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        jac::Logger::error(
            std::format("Failed to open file stream: {}", filename.c_str())
                .c_str());
        return false;
    }

    auto fileSize = fs::file_size(filepath, ec);
    if (ec) {
        ESP_LOGE(TAG, "Failed to get file size: %s", filename.c_str());
        return false;
    }

    if (fileSize == 0) {
        ESP_LOGE(TAG, "File is empty: %s", filename.c_str());
        return false;
    }

    buffer.resize(fileSize);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);

    if (file.fail()) {
        ESP_LOGE(TAG, "Error reading data from file: %s", filename.c_str());
        return false;
    }

    ESP_LOGI(TAG, "Successfully read %s, size: %zu bytes", filename.c_str(),
             (size_t)fileSize);
    return true;
}

uint16_t Texture::getUint16(const uint8_t *data, size_t offset,
                            bool littleEndian) {
    if (littleEndian) {
        return data[offset] | (data[offset + 1] << 8);
    } else {
        return (data[offset] << 8) | data[offset + 1];
    }
}

uint32_t Texture::getUint32(const uint8_t *data, size_t offset,
                            bool littleEndian) {
    if (littleEndian) {
        return data[offset] | (data[offset + 1] << 8) |
               (data[offset + 2] << 16) | (data[offset + 3] << 24);
    } else {
        return (data[offset] << 24) | (data[offset + 1] << 16) |
               (data[offset + 2] << 8) | data[offset + 3];
    }
}

int32_t Texture::getInt32(const uint8_t *data, size_t offset,
                          bool littleEndian) {
    return static_cast<int32_t>(getUint32(data, offset, littleEndian));
}

bool Texture::fromBMP(const std::string &filename, Texture &outTexture,
                      bool littleEndian) {
    std::vector<uint8_t> fileData;
    if (!readFile(filename, fileData)) {
        jac::Logger::error(
            std::format("Failed to read BMP file: {}", filename.c_str()));
        return false;
    }

    const uint8_t *data = fileData.data();

    if (fileData.size() < 54) {
        jac::Logger::error("File too small to be a BMP");
        return false;
    }

    uint16_t signature = getUint16(data, 0, littleEndian);
    if (signature != 0x4D42) { // 'BM'
        jac::Logger::error(
            std::format("Invalid BMP file signature: 0x{}", signature));
        return false;
    }

    uint32_t pixelDataOffset = getUint32(data, 10, littleEndian);
    uint32_t headerSize = getUint32(data, 14, littleEndian);
    int32_t width = getInt32(data, 18, littleEndian);
    int32_t height = getInt32(data, 22, littleEndian);
    uint16_t bitsPerPixel = getUint16(data, 28, littleEndian);

    jac::Logger::debug(std::format(
        "BMP info: {}x{}, {} bpp, header: {} bytes, data offset: 0x{}",
        (int)width, (int)height, (int)bitsPerPixel, (int)headerSize,
        (unsigned int)pixelDataOffset));

    if (width <= 0 || height <= 0) {
        jac::Logger::error(std::format("Invalid BMP dimensions: {}x{}",
                                       (int)width, (int)height));
        return false;
    }

    if (pixelDataOffset >= fileData.size()) {
        jac::Logger::error("Pixel data offset out of bounds");
        return false;
    }

    std::vector<std::vector<Color>> pixels;

    if (bitsPerPixel == 24) {
        int bytesPerRow = ((width * 3 + 3) / 4) * 4;

        pixels.resize(height);
        for (int y = 0; y < height; y++) {
            pixels[y].resize(width);
            for (int x = 0; x < width; x++) {
                size_t offset =
                    pixelDataOffset + (height - 1 - y) * bytesPerRow + x * 3;

                if (offset + 2 >= fileData.size()) {
                    jac::Logger::error("BMP data out of bounds");
                    return false;
                }

                uint8_t b = data[offset];
                uint8_t g = data[offset + 1];
                uint8_t r = data[offset + 2];
                pixels[y][x] = Color(r, g, b, 1.0f);
            }
        }
    } else if (bitsPerPixel == 32) {
        pixels.resize(height);
        for (int y = 0; y < height; y++) {
            pixels[y].resize(width);
            for (int x = 0; x < width; x++) {
                size_t offset =
                    pixelDataOffset + (height - 1 - y) * width * 4 + x * 4;

                if (offset + 3 >= fileData.size()) {
                    jac::Logger::error("BMP data out of bounds");
                    return false;
                }

                uint8_t b = data[offset];
                uint8_t g = data[offset + 1];
                uint8_t r = data[offset + 2];
                uint8_t a = data[offset + 3];
                pixels[y][x] = Color(r, g, b, a / 255.0f);
            }
        }
    } else {
        jac::Logger::error(std::format(
            "Unsupported BMP format: {} bits per pixel", bitsPerPixel));
        return false;
    }

    outTexture = Texture(pixels);
    jac::Logger::debug("Texture loaded successfully");
    return true;
}

Color Texture::sample(int u, int v) const {
    if (!valid || pixels.empty() || width == 0 || height == 0) {
        return Color(0, 0, 0, 1.0f);
    }
    int x = u;
    int y = v;

    if (wrapMode == "repeat") {
        if (u >= 0 && u < width && v >= 0 && v < height) {
            return pixels[y][x];
        } else {
            x = u % width;
            y = v % height;
            if (x < 0)
                x += width;
            if (y < 0)
                y += height;
        }
    } else {
        x = std::max(0, std::min(width - 1, x));
        y = std::max(0, std::min(height - 1, y));
    }

    if (y >= 0 && y < height && x >= 0 && x < width) {
        return pixels[y][x];
    }
    return Color(0, 0, 0, 1.0f);
}

void Texture::setWrapMode(const std::string &mode) {
    if (mode == "repeat" || mode == "clamp") {
        wrapMode = mode;
    } else {
        ESP_LOGW(TAG, "Invalid wrap mode: %s, using 'repeat'", mode.c_str());
        wrapMode = "repeat";
    }
}
