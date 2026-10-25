#include "bmp_image.h"
#include "pack_defines.h"

#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t signature;
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t image_size;
    int32_t x_pixels_per_meter;
    int32_t y_pixels_per_meter;
    int32_t colors_used;
    int32_t colors_important;
}
PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    const int w = image.GetWidth();
    const int h = image.GetHeight();
    const int stride = GetBMPStride(w);
    const uint32_t image_size = static_cast<uint32_t>(stride * h);

    BitmapFileHeader file_header;
    file_header.signature = 0x4D42; // "BM"
    file_header.file_size = static_cast<uint32_t>(
        sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + image_size
    );
    file_header.reserved = 0;
    file_header.data_offset = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

    BitmapInfoHeader info_header;
    info_header.header_size = sizeof(BitmapInfoHeader);
    info_header.width = w;
    info_header.height = h;
    info_header.planes = 1;
    info_header.bits_per_pixel = 24;
    info_header.compression = 0;
    info_header.image_size = image_size;
    info_header.x_pixels_per_meter = 11811;
    info_header.y_pixels_per_meter = 11811;
    info_header.colors_used = 0;
    info_header.colors_important = 0x1000000;

    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    vector<char> buffer(stride, 0);

    for (int y = h - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);

        for (int x = 0; x < w; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }

        out.write(buffer.data(), stride);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    if (!in) {
        return {};
    }

    if (file_header.signature != 0x4D42) {
        return {};
    }

    if (file_header.data_offset != sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader)) {
        return {};
    }

    if (info_header.header_size != sizeof(BitmapInfoHeader)) {
        return {};
    }

    if (info_header.planes != 1) {
        return {};
    }

    if (info_header.bits_per_pixel != 24) {
        return {};
    }

    if (info_header.compression != 0) {
        return {};
    }

    if (info_header.width <= 0 || info_header.height <= 0) {
        return {};
    }

    const int w = info_header.width;
    const int h = info_header.height;
    const int stride = GetBMPStride(w);

    Image result(w, h, Color::Black());
    vector<char> buffer(stride);

    for (int y = h - 1; y >= 0; --y) {
        in.read(buffer.data(), stride);
        if (!in) {
            return {};
        }

        Color* line = result.GetLine(y);
        for (int x = 0; x < w; ++x) {
            line[x].b = static_cast<byte>(static_cast<unsigned char>(buffer[x * 3 + 0]));
            line[x].g = static_cast<byte>(static_cast<unsigned char>(buffer[x * 3 + 1]));
            line[x].r = static_cast<byte>(static_cast<unsigned char>(buffer[x * 3 + 2]));
            line[x].a = byte{255};
        }
    }

    return result;
}

}  // namespace img_lib