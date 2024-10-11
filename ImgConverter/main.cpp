#include <img_lib.h>
#include <bmp_image.h>
#include <jpeg_image.h>
#include <ppm_image.h>

#include <filesystem>
#include <string_view>
#include <iostream>

using namespace std;

enum class Format {
    BMP,
    JPEG,
    PPM,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const string ext = input_file.extension().string();
    if (ext == ".bmp"sv) {
        return Format::BMP;
    }
    
    if (ext == ".jpg"sv || ext == ".jpeg"sv) {
        return Format::JPEG;
    }

    if (ext == ".ppm"sv) {
        return Format::PPM;
    }

    return Format::UNKNOWN;
}

class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
};

namespace image_format {

    class BMP : public ImageFormatInterface {
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SaveBMP(file, image);
        }

        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadBMP(file);
        }
    };

    class JPEG : public ImageFormatInterface {
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SaveJPEG(file, image);
        }

        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadJPEG(file);
        }
    };

    class PPM : public ImageFormatInterface {        
    public:
        bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
            return img_lib::SavePPM(file, image);
        }
        img_lib::Image LoadImage(const img_lib::Path& file) const override {
            return img_lib::LoadPPM(file);
        }
    };

} // namespace image_format

ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    static image_format::BMP bmp_;
    static image_format::JPEG jpeg_;
    static image_format::PPM ppm_;    

    switch (GetFormatByExtension(path)) {
        case Format::BMP:
            return &bmp_;

        case Format::JPEG:
            return &jpeg_;
            
        case Format::PPM:
            return &ppm_;

        case Format::UNKNOWN:
            return nullptr;

        default:
            return nullptr;
    }
}

int main(int argc, const char** argv) {
    if (argc != 3) {
        cerr << "Usage: "sv << argv[0] << " <in_file> <out_file>"sv << endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    img_lib::Path out_path = argv[2];

    auto* input_format = GetFormatInterface(in_path);

    if (!input_format) {
        cerr << "Unknown format of the input file"sv << endl;
        return 2;
    }

    auto* output_format = GetFormatInterface(out_path);

    if (!output_format) {
        cerr << "Unknown format of the output file"sv << endl;
        return 3;
    }

    img_lib::Image image = input_format->LoadImage(in_path);

    if (!image) {
        cerr << "Loading failed"sv << endl;
        return 4;
    }

    if (!output_format->SaveImage(out_path, image)) {
        cerr << "Saving failed"sv << endl;
        return 5;
    }

    cout << "Successfully converted"sv << endl;
}