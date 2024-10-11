#include "ppm_image.h"

#include <array>
#include <fstream>
#include <string_view>

using namespace std;

namespace img_lib {

static const string_view PPM_SIG = "P6"sv;
static const int PPM_MAX = 255;

bool SavePPM(const Path& file, const Image& image) {
    ofstream ofs(file, ios::binary);
    if (!ofs) return false;

    int height = image.GetHeight();
    int width = image.GetWidth();
    vector<char> buffer(width * 3);

    ofs << PPM_SIG << '\n' << width << ' ' << height << '\n' << PPM_MAX << '\n';
    for (int i = 0; i < height; ++i) {
        const auto line = image.GetLine(i);        
        for (int j = 0; j < width; ++j) {
            buffer[j * 3 + 0] = static_cast<char>(line[j].r);
            buffer[j * 3 + 1] = static_cast<char>(line[j].g);
            buffer[j * 3 + 2] = static_cast<char>(line[j].b);
        }
        ofs.write(buffer.data(), buffer.size());        
    } 

    ofs.close();
    return true;
}

Image LoadPPM(const Path& file) {
    // открываем поток с флагом ios::binary
    // поскольку будем читать данные в двоичном формате
    ifstream ifs(file, ios::binary);
    std::string sign;
    int w, h, color_max;

    // читаем заголовок: он содержит формат, размеры изображения
    // и максимальное значение цвета
    ifs >> sign >> w >> h >> color_max;

    // мы поддерживаем изображения только формата P6
    // с максимальным значением цвета 255
    if (sign != PPM_SIG || color_max != PPM_MAX) {
        return {};
    }

    // пропускаем один байт - это конец строки
    const char next = ifs.get();
    if (next != '\n') {
        return {};
    }

    Image result(w, h, Color::Black());
    std::vector<char> buff(w * 3);

    for (int y = 0; y < h; ++y) {
        Color* line = result.GetLine(y);
        ifs.read(buff.data(), w * 3);

        for (int x = 0; x < w; ++x) {
            line[x].r = static_cast<byte>(buff[x * 3 + 0]);
            line[x].g = static_cast<byte>(buff[x * 3 + 1]);
            line[x].b = static_cast<byte>(buff[x * 3 + 2]);
        }
    }

    return result;
}

}  // namespace img_lib