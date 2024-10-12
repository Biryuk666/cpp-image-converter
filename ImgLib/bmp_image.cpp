#include "bmp_image.h"
#include "pack_defines.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {   
    // Подпись — 2 байта. Символы BM
    uint8_t signature[2] = {'B', 'M'};

    // Суммарный размер заголовка и данных — 4 байта.
    // Размер данных определяется как отступ, умноженный на высоту изображения
    uint32_t header_summary;

    // Зарезервированное пространство, заполненные нулями — 4 байта
    uint8_t reserved[4];

    // Отступ данных от начала файла — 4 байта. Равен размеру двух частей заголовка.
    uint32_t padding;
} PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    // Размер заголовка — 4 байта. Учитывается только размер второй части заголовка
    uint32_t header_size = sizeof(BitmapInfoHeader);

    // Ширина изображения в пикселях — 4 байта
    int32_t image_width;

    // Высота изображения в пикселях — 4 байта
    int32_t image_height;

    // Количество плоскостей — 2 байта. В нашем случае всегда 1 — одна RGB плоскость
    uint16_t plane_count = 1;

    // Количество бит на пиксель — 2 байта. В нашем случае всегда 24
    uint16_t bits_per_pixel = 24;

    // Тип сжатия — 4 байта. В нашем случае всегда 0 — отсутствие сжатия
    uint32_t compression_type = 0;

    // Количество байт в данных — 4 байта. Произведение отступа на высоту
    uint32_t bytes_in_data;

    // Горизонтальное разрешение, пикселей на метр — 4 байта. В нашем случае всегда 11811, что примерно соответствует 300 DPI
    int32_t horizontal_resolution = 11811;

    // Вертикальное разрешение, пикселей на метр — 4 байта. В нашем случае всегда 11811, что примерно соответствует 300 DPI
    int32_t vertical_resolution = 11811;

    // Количество использованных цветов — 4 байта. В нашем случае всегда 0 — значение не определено
    int32_t used_colors_count = 0;

    // Количество значимых цветов — 4 байта. В нашем случае всегда 0x1000000
    int32_t significant_colors_count = 0x1000000;
} PACKED_STRUCT_END

static int GetBMPStride(int w) {
    return 4 * ((w * 3 + 3) / 4);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    if (!ofs) return false;

    int height = image.GetHeight();
    int width = image.GetWidth();    
    if (height <= 0 || width <= 0) return false; 
    int bmp_stride = GetBMPStride(width);

    BitmapFileHeader bitmap_file_header;
    bitmap_file_header.padding = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    bitmap_file_header.header_summary =  bitmap_file_header.padding + bmp_stride * height;
    ofs.write(reinterpret_cast<char*>(&bitmap_file_header), sizeof(BitmapFileHeader));
    
    BitmapInfoHeader bitmap_info_header;
    bitmap_info_header.image_width = width;
    bitmap_info_header.image_height = height;
    bitmap_info_header.bytes_in_data = bmp_stride * height;    
    ofs.write(reinterpret_cast<char*>(&bitmap_info_header), sizeof(BitmapInfoHeader));
    
    vector<char> buffer(bmp_stride);
    for (int y = height - 1; y >= 0; y--) {
        const Color* line = image.GetLine(y);
        for (int x = 0; x < width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b);
            buffer[x * 3 + 1] = static_cast<char>(line[x].g);
            buffer[x * 3 + 2] = static_cast<char>(line[x].r);
        }
        ofs.write(buffer.data(), bmp_stride);
    }

    ofs.close();
    return true;
}

Image LoadBMP(const Path& file) {
    ifstream ifs(file, ios::binary);
    if (!ifs) return {};

    BitmapFileHeader bitmap_file_header;
    ifs.read(reinterpret_cast<char*>(&bitmap_file_header), sizeof(BitmapFileHeader));
    if (bitmap_file_header.signature[0] != 'B' || bitmap_file_header.signature[1] != 'M')  return {};
    
    BitmapInfoHeader bitmap_info_header;
    ifs.read(reinterpret_cast<char*>(&bitmap_info_header), sizeof(BitmapInfoHeader));

    Image result(bitmap_info_header.image_width, bitmap_info_header.image_height, Color::Black());
    int height = result.GetHeight();
    int width = result.GetWidth();   
    int bmp_stride = GetBMPStride(width);

    vector<char> buffer(bmp_stride);
    for (int y = height - 1; y >= 0; y--) {
        auto line = result.GetLine(y);
        ifs.read(buffer.data(), bmp_stride);
        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]);
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]);
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]);
        }
    }
    
    return result;
}

}  // namespace img_lib