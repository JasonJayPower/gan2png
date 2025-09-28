#pragma once

#include <cstdio>
#include <cstring>
#include <filesystem>

#include "types.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Constants
inline constexpr u16 SIGNATURE_OFFSET = 0x000;
inline constexpr u16 SIGNATURE_LENGTH = 4;
inline constexpr u16 WIDTH_OFFSET     = 0x424;
inline constexpr u16 HEIGHT_OFFSET    = 0x426;
inline constexpr u16 PALETTE_OFFSET   = 0x430;
inline constexpr u16 IMAGE_OFFSET     = 0x470;

inline constexpr u16 PALETTE_SIZE       = 64;
inline constexpr u16 PIXEL_STRIDE       = 4;
inline constexpr u16 NUM_PALETTE_COLORS = PALETTE_SIZE / PIXEL_STRIDE;

struct Buffer {
    std::unique_ptr<u8[]> data = nullptr;
    u64 size                   = 0;
};

Buffer readFromFile(const char* filepath)
{
    FILE* file = nullptr;
    fopen_s(&file, filepath, "rb");

    if (!file) {
        printf("Failed to load file\n");
        return {};
    }

    fseek(file, 0, SEEK_END);
    const u64 fileSize = static_cast<u64>(ftell(file));
    fseek(file, 0, SEEK_SET);

    Buffer buffer{ .data = std::make_unique<u8[]>(fileSize), .size = fileSize };
    const u64 readSize = fread(buffer.data.get(), 1, fileSize, file);
    fclose(file);

    if (readSize != fileSize) {
        printf("File read incomplete\n");
        return {};
    }

    return buffer;
}

bool validate_file(const Buffer& buffer)
{
    static constexpr u8 ExpectedSignature[SIGNATURE_LENGTH] = { 0x47, 0x41, 0x4E, 0x10 };
    for (u64 i = 0; i < SIGNATURE_LENGTH; ++i) {
        if (buffer.data[SIGNATURE_OFFSET + i] != ExpectedSignature[i]) {
            printf("Invalid file %llu\n", i);
            return false;
        }
    }
    return true;
}

bool to_png(const std::filesystem::path& path)
{
    if (!std::filesystem::is_directory(path)) {
        printf("Invalid directory: %s\n", path.string().c_str());
        return false;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const auto& file_path = entry.path();

        Buffer fileBuffer = readFromFile(file_path.string().c_str());
        if (!validate_file(fileBuffer)) {
            printf("Skipping invalid file: %s\n", file_path.filename().string().c_str());
            continue;
        }

        u8 palette[NUM_PALETTE_COLORS][PIXEL_STRIDE] = { { 0, 0, 0, 0 } };
        for (u64 i = 1; i < NUM_PALETTE_COLORS; ++i) {
            const u64 offset = PALETTE_OFFSET + i * PIXEL_STRIDE;
            palette[i][0]    = fileBuffer.data[offset + 0];
            palette[i][1]    = fileBuffer.data[offset + 1];
            palette[i][2]    = fileBuffer.data[offset + 2];
            palette[i][3]    = 0XFF;
        }

        const u16 width  = fileBuffer.data[WIDTH_OFFSET] | (fileBuffer.data[WIDTH_OFFSET + 1] << 8);
        const u16 height = fileBuffer.data[HEIGHT_OFFSET] | (fileBuffer.data[HEIGHT_OFFSET + 1] << 8);
        const u32 count  = width * height;

        std::unique_ptr<u8[]> pixeldata = std::make_unique<u8[]>(PIXEL_STRIDE * count);

        for (u64 y = 0; y < height; ++y) {
            for (u64 x = 0; x < width; x += 2) {
                const u64 i1 = y * width + x;
                const u64 i2 = i1 + 1;

                const u8 byte   = fileBuffer.data[IMAGE_OFFSET + (i1 >> 1)];
                const u8 index1 = byte & 0x0F;
                const u8 index2 = byte >> 4;

                std::memcpy(&pixeldata[i1 * PIXEL_STRIDE], palette[index1], PIXEL_STRIDE);
                std::memcpy(&pixeldata[i2 * PIXEL_STRIDE], palette[index2], PIXEL_STRIDE);
            }
        }

        const auto output_path = file_path.parent_path() / (file_path.stem().string() + ".png");
        stbi_write_png(
            output_path.string().c_str(), width, height, PIXEL_STRIDE, pixeldata.get(), width * PIXEL_STRIDE);
    }
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printf("Usage: %s <path>\n", argv[0]);
        return 1;
    }

    bool result = to_png(std::filesystem::canonical(argv[0]).parent_path() / argv[1]);
    return 0;
}
