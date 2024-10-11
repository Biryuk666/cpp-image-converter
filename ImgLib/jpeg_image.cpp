#include "ppm_image.h"

#include <array>
#include <fstream>
#include <stdio.h>
#include <setjmp.h>
#include <vector>

#include <jpeglib.h>

using namespace std;

namespace img_lib {

// структура из примера LibJPEG
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

// функция из примера LibJPEG
METHODDEF(void)
my_error_exit (j_common_ptr cinfo) {
    my_error_ptr myerr = (my_error_ptr) cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

// тип JSAMPLE фактически псевдоним для unsigned char
void SaveScanlineToImage(const JSAMPLE* row, int y, Image& out_image) {
    Color* line = out_image.GetLine(y);
    for (int x = 0; x < out_image.GetWidth(); ++x) {
        const JSAMPLE* pixel = row + x * 3;
        line[x] = Color{byte{pixel[0]}, byte{pixel[1]}, byte{pixel[2]}, byte{255}};
    }
}

bool SaveJPEG(const Path& file, const Image& image) {
    struct jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    FILE * outfile;
    JSAMPROW row_pointer[1];
    int row_stride = image.GetWidth() * 3;

    int height = image.GetHeight();
    int width = image.GetWidth();

    /* Шаг 1: выделяем память и инициализируем объект кодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    /* Шаг 2: устанавливаем пункт назначения данных */

#ifdef _MSC_VER
    if ((outfile = _wfopen(file.wstring().c_str(), "wb")) == NULL) {
#else
    if ((outfile = fopen(file.string().c_str(), "wb")) == NULL) {
#endif
    return false;
    }

    jpeg_stdio_dest(&cinfo, outfile);

    /* Шаг 3: устанавливаем параметры кодирования */
    
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);

    /* Шаг 4: начинаем кодирование */

    jpeg_start_compress(&cinfo, TRUE);

    /* Шаг 5: while (остаются строки изображения) */
    /*                     jpeg_write_scanlines(...); */

    vector<JSAMPLE> buffer(row_stride);

    while (cinfo.next_scanline < cinfo.image_height) {
        for (int i = 0; i < width; ++i)  {
            auto line = image.GetLine(cinfo.next_scanline);
            buffer[3 * i + 0] = static_cast<JSAMPLE>(line[i].r);
            buffer[3 * i + 1] = static_cast<JSAMPLE>(line[i].g);
            buffer[3 * i + 2] = static_cast<JSAMPLE>(line[i].b);
        }

        row_pointer[0] = &buffer[0];
        (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    /* Шаг 6: Останавливаем кодирование */

    jpeg_finish_compress(&cinfo);
    fclose(outfile);

    /* Шаг 7: Выгружаем JPEG кодированный объект */

    jpeg_destroy_compress(&cinfo);
    
    return true;
}

Image LoadJPEG(const Path& file) {
    jpeg_decompress_struct cinfo;
    my_error_mgr jerr;
    
    FILE* infile;
    JSAMPARRAY buffer;
    int row_stride;

    // Тут не избежать функции открытия файла из языка C,
    // поэтому приходится использовать конвертацию пути к string.
    // Под Visual Studio это может быть опасно, и нужно применить
    // нестандартную функцию _wfopen
#ifdef _MSC_VER
    if ((infile = _wfopen(file.wstring().c_str(), "rb")) == NULL) {
#else
    if ((infile = fopen(file.string().c_str(), "rb")) == NULL) {
#endif
    return {};
    }

    /* Шаг 1: выделяем память и инициализируем объект декодирования JPEG */

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return {};
    }

    jpeg_create_decompress(&cinfo);

    /* Шаг 2: устанавливаем источник данных */

    jpeg_stdio_src(&cinfo, infile);

    /* Шаг 3: читаем параметры изображения через jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);

    /* Шаг 4: устанавливаем параметры декодирования */

    // установим желаемый формат изображения
    cinfo.out_color_space = JCS_RGB;
    cinfo.output_components = 3;

    /* Шаг 5: начинаем декодирование */

    (void) jpeg_start_decompress(&cinfo);
    
    row_stride = cinfo.output_width * cinfo.output_components;
    
    buffer = (*cinfo.mem->alloc_sarray)
                ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Шаг 5a: выделим изображение ImgLib */
    Image result(cinfo.output_width, cinfo.output_height, Color::Black());

    /* Шаг 6: while (остаются строки изображения) */
    /*                     jpeg_read_scanlines(...); */

    while (cinfo.output_scanline < cinfo.output_height) {
        int y = cinfo.output_scanline;
        (void) jpeg_read_scanlines(&cinfo, buffer, 1);

        SaveScanlineToImage(buffer[0], y, result);
    }

    /* Шаг 7: Останавливаем декодирование */

    (void) jpeg_finish_decompress(&cinfo);

    /* Шаг 8: Освобождаем объект декодирования */

    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return result;
}

} // of namespace img_lib