#ifndef BMP_HEADER
#define BMP_HEADER

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

#pragma pack(push, 1)

struct BMPFileHeader {
    uint16_t file_type{0x4D42};
    uint32_t file_size{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t offset_data{0};
} __attribute ((__packed__));

struct BMPInfoHeader {
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bit_count{0};
    uint32_t compression{0};
    uint32_t size_image{0};
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
} __attribute ((__packed__));

struct BMPColorHeader {
    uint32_t red_mask{0x00ff0000};          // Bit mask for the red channel
    uint32_t green_mask{0x0000ff00};        // Bit mask for the green channel
    uint32_t blue_mask{0x000000ff};         // Bit mask for the blue channel
    uint32_t alpha_mask{0xff000000};        // Bit mask for the alpha channel
    uint32_t color_space_type{0x73524742};  // Default "sRGB" (0x73524742)
    uint32_t unused[16]{0};                 // Unused data for sRGB color space
} __attribute ((__packed__));

#pragma pack(pop)

struct BMP {
    BMPFileHeader           file_header;
    BMPInfoHeader           bmp_info_header;
    BMPColorHeader          bmp_color_header;
    std::vector<uint8_t>    data;

    BMP() = default;

    BMP(const char *fname) {
        read(fname);
    }

    void read(const char *fname) {
        std::ifstream inp{ fname, std::ios_base::binary };
        if (inp) {
            inp.read((char*) &file_header, sizeof(file_header));
            if (file_header.file_type != 0x4D42) {
                throw std::runtime_error("Error! Unrecognized file format.");
            }

            inp.read((char*) &bmp_info_header, sizeof(bmp_info_header));

            if (bmp_info_header.bit_count == 32) {
                if (bmp_info_header.size >= (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) {
                    inp.read((char*) &bmp_color_header, sizeof(bmp_color_header));
                    check_color_header(bmp_color_header);
                } else {
                    std::cerr << "Error! The file \"" << fname << "\" does not seem to contain bit mask information\n";
                    throw std::runtime_error("Error! Unrecognized file format.");
                }
            }

            inp.seekg(file_header.offset_data, inp.beg);

            if (bmp_info_header.bit_count == 32) {
                bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            } else {
                bmp_info_header.size = sizeof(BMPInfoHeader);
                file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
            }
            file_header.file_size = file_header.offset_data;

            if (bmp_info_header.height < 0) {
                throw std::runtime_error("The program can treat only BMP images with the origin in the bottom left corner!");
            }

            data.resize(bmp_info_header.width * bmp_info_header.height * bmp_info_header.bit_count / 8);

            if (bmp_info_header.width % 4 == 0) {
                inp.read((char*) data.data(), data.size());
                file_header.file_size += static_cast<uint32_t> (data.size());
            } else {
                row_stride = bmp_info_header.width * bmp_info_header.bit_count / 8;
                uint32_t new_stride = make_stride_aligned(4);
                std::vector<uint8_t> padding_row(new_stride - row_stride);

                for (int y = 0; y < bmp_info_header.height; ++y) {
                    inp.read((char*) (data.data() + row_stride * y), row_stride);
                    inp.read((char*) padding_row.data(), padding_row.size());
                }

                file_header.file_size += static_cast<uint32_t> (data.size()) +
                            bmp_info_header.height * static_cast<uint32_t> (padding_row.size());
            }
        } else {
            throw std::runtime_error("Unable to open the input image file.");
        }
    }

    BMP(int32_t width, int32_t height, bool has_alpha = true) {
        if (width <= 0 || height <= 0) {
            throw std::runtime_error("The image width and height must be positive numbers.");
        }

        bmp_info_header.width = width;
        bmp_info_header.height = height;

        if (has_alpha) {
            bmp_info_header.size = sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader);

            bmp_info_header.bit_count = 32;
            bmp_info_header.compression = 3;
            row_stride = width * 4;
            data.resize(row_stride * height);
            file_header.file_size = file_header.offset_data + data.size();
        } else {
            bmp_info_header.size = sizeof(BMPInfoHeader);
            file_header.offset_data = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);

            bmp_info_header.bit_count = 24;
            bmp_info_header.compression = 0;
            row_stride = width * 3;
            data.resize(row_stride * height);

            uint32_t new_stride = make_stride_aligned(4);
            file_header.file_size = file_header.offset_data +
                        static_cast<uint32_t> (data.size()) + bmp_info_header.height * (new_stride - row_stride);
        }
    }

    void write(const char *fname) {
        std::ofstream of{fname, std::ios_base::binary};

        if (of) {
            if (bmp_info_header.bit_count == 32) {
                write_headers_and_data(of);
            } else if (bmp_info_header.bit_count == 24) {
                if (bmp_info_header.width % 4 == 0) {
                    write_headers_and_data(of);
                } else {
                    uint32_t new_stride = make_stride_aligned(4);
                    std::vector<uint8_t> padding_row(new_stride - row_stride);

                    write_headers(of);

                    for (int y = 0; y < bmp_info_header.height; ++y) {
                        of.write((const char*) (data.data() + row_stride * y), row_stride);
                        of.write((const char*) padding_row.data(), padding_row.size());
                    }
                }
            } else {
                throw std::runtime_error("The program can treat only 24 or 32 bits per pixel BMP files");
            }
        } else {
            throw std::runtime_error("Unable to open the output image file.");
        }
    }

    void fill_region(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h, uint8_t B, uint8_t G, uint8_t R, uint8_t A = 1) {
        if (x0 + w > (uint32_t) bmp_info_header.width || y0 + h > (uint32_t) bmp_info_header.height) {
            throw std::runtime_error("The region does not fit in the image!");
        }

        uint32_t channels = bmp_info_header.bit_count / 8;
        for (uint32_t y = y0; y < y0 + h; ++y) {
            for (uint32_t x = x0; x < x0 + w; ++x) {
                data[channels * (y * bmp_info_header.width + x) + 0] = B;
                data[channels * (y * bmp_info_header.width + x) + 1] = G;
                data[channels * (y * bmp_info_header.width + x) + 2] = R;

                if (channels == 4) {
                    data[channels * (y * bmp_info_header.width + x) + 3] = A;
                }
            }
        }
    }

    // -----/ FILTER FUNCTIONS /-----

    void get_pixel(uint32_t x0, uint32_t y0, uint8_t *R, uint8_t *G, uint8_t *B, uint8_t *A) {
        if (x0 >= (uint32_t) bmp_info_header.width || y0 >= (uint32_t) bmp_info_header.height || x0 < 0 || y0 < 0) {
            throw std::runtime_error("The point is outside the image boundaries!");
        }

        uint32_t channels = bmp_info_header.bit_count / 8;
        *B = data[channels * (y0 * bmp_info_header.width + x0) + 0];
        *G = data[channels * (y0 * bmp_info_header.width + x0) + 1];
        *R = data[channels * (y0 * bmp_info_header.width + x0) + 2];
        if (channels == 4) {
            *A = data[channels * (y0 * bmp_info_header.width + x0) + 3];
        }
    }

    int check_pixel(uint32_t x0, uint32_t y0, uint8_t R, uint8_t G, uint8_t B, uint8_t A) {
        if (x0 >= (uint32_t) bmp_info_header.width || y0 >= (uint32_t) bmp_info_header.height || x0 < 0 || y0 < 0) {
            throw std::runtime_error("The point is outside the image boundaries!");
        }

        uint32_t channels = bmp_info_header.bit_count / 8;
        if (data[channels * (y0 * bmp_info_header.width + x0) + 0] != B) {
            return 0;
        }
        if (data[channels * (y0 * bmp_info_header.width + x0) + 1] != G) {
            return 0;
        }
        if (data[channels * (y0 * bmp_info_header.width + x0) + 2] != R) {
            return 0;
        }
        if (channels == 4 && data[channels * (y0 * bmp_info_header.width + x0) + 3] != A) {
            return 0;
        }

        return 1;
    }

    void set_pixel(uint32_t x0, uint32_t y0, uint8_t R, uint8_t G, uint8_t B, uint8_t A) {
        if (x0 >= (uint32_t) bmp_info_header.width || y0 >= (uint32_t) bmp_info_header.height || x0 < 0 || y0 < 0) {
            throw std::runtime_error("The point is outside the image boundaries!");
        }

        uint32_t channels = bmp_info_header.bit_count / 8;
        data[channels * (y0 * bmp_info_header.width + x0) + 0] = B;
        data[channels * (y0 * bmp_info_header.width + x0) + 1] = G;
        data[channels * (y0 * bmp_info_header.width + x0) + 2] = R;

        if (channels == 4) {
            data[channels * (y0 * bmp_info_header.width + x0) + 3] = A;
        }
    }

    void negative() {
        uint32_t channels = bmp_info_header.bit_count / 8;

        for (uint32_t x0 = 0; x0 < (uint32_t) bmp_info_header.width; ++x0) {
            for (uint32_t y0 = 0; y0 < (uint32_t) bmp_info_header.height; ++y0) {
                data[channels * (y0 * bmp_info_header.width + x0) + 0] = 255 -
                            data[channels * (y0 * bmp_info_header.width + x0) + 0];
                data[channels * (y0 * bmp_info_header.width + x0) + 1] = 255 -
                            data[channels * (y0 * bmp_info_header.width + x0) + 1];
                data[channels * (y0 * bmp_info_header.width + x0) + 2] = 255 -
                            data[channels * (y0 * bmp_info_header.width + x0) + 2];
            }
        }
    }

    size_t replace_color(uint8_t R1, uint8_t G1, uint8_t B1, uint8_t A1, uint8_t R2, uint8_t G2, uint8_t B2, uint8_t A2 = 1) {
        size_t changed_pixels_counter = 0;

        for (uint32_t x0 = 0; x0 < (uint32_t) bmp_info_header.width; ++x0) {
            for (uint32_t y0 = 0; y0 < (uint32_t) bmp_info_header.height; ++y0) {
                if (check_pixel(x0, y0, R1, G1, B1, A1)) {
                    set_pixel(x0, y0, R2, G2, B2, A2);
                    ++changed_pixels_counter;
                }
            }
        }

        // printf("%zd pixels have changed!\n", changed_pixels_counter);
        return changed_pixels_counter;
    }

    void clarity(double div = 8) {
        // div <=> clarity force
        uint32_t channels = bmp_info_header.bit_count / 8;
        struct clarity_matrix c_mx;
        double new_pixel;
        std::vector<uint8_t> new_data = data;

        for (int32_t x0 = 0; x0 < (int32_t) bmp_info_header.width; ++x0) {
            for (int32_t y0 = 0; y0 < (int32_t) bmp_info_header.height; ++y0) {
                for (uint32_t k = 0; k < channels; ++k) {
                    int central_clarity_coeff = 8;
                    if (!x0 || x0 == bmp_info_header.width - 1) {
                        central_clarity_coeff -= 3;
                    }
                    if (!y0 || y0 == bmp_info_header.height - 1) {
                        central_clarity_coeff -= 3;
                    }
                    if (central_clarity_coeff == 3) {
                        ++central_clarity_coeff;
                    }
                    new_pixel = data[channels * ((y0) * bmp_info_header.width + (x0)) + k];
                    new_pixel += (data[channels * ((y0) * bmp_info_header.width + (x0)) + k] * central_clarity_coeff) / div;

                    for (int32_t i = -std::min(c_mx.deviation, bmp_info_header.height - y0 - 1);
                                    i <= std::min(c_mx.deviation, y0); ++i) {
                        for (int32_t j = std::max(-c_mx.deviation, -x0); j <= 
                                        std::min(c_mx.deviation, bmp_info_header.width - x0 - 1); ++j) {
                            if (!i && !j) {
                                continue;
                            }
                            
                            if (data[channels * ((y0 - i) * bmp_info_header.width + (x0 + j)) + k] > new_pixel) {
                                new_pixel = 0;
                                break;
                            }
                            new_pixel += (data[channels * ((y0 - i) * bmp_info_header.width + (x0 + j)) + k] * 
                                            c_mx.data[c_mx.deviation + i][c_mx.deviation + j]) / div;
                        }
                    }
                    
                    if (new_pixel != 0) {
                        new_data[channels * (y0 * bmp_info_header.width + x0) + k] = (uint8_t) new_pixel;
                    }
                }
            }
        }

        data = new_data;
        // median_filter(1);
    }

    void gauss() {
        uint32_t channels = bmp_info_header.bit_count / 8;
        struct gauss_matrix g_mx;
        uint8_t new_pixel;
        std::vector<uint8_t> new_data = data;

        for (int32_t x0 = 0; x0 < (int32_t) bmp_info_header.width; ++x0) {
            for (int32_t y0 = 0; y0 < (int32_t) bmp_info_header.height; ++y0) {
                for (uint32_t k = 0; k < channels; ++k) {
                    new_pixel = 0;
                    for (int32_t i = -std::min(g_mx.deviation, bmp_info_header.height - y0 - 1); 
                                    i <= std::min(g_mx.deviation, y0); ++i) {
                        for (int32_t j = std::max(-g_mx.deviation, -x0); j <= 
                                        std::min(g_mx.deviation, bmp_info_header.width - x0 - 1); ++j) {
                            new_pixel += data[channels * ((y0 - i) * bmp_info_header.width + (x0 + j)) + k] * 
                                            g_mx.data[g_mx.deviation + i][g_mx.deviation + j];
                        }
                    }
                    new_data[channels * (y0 * bmp_info_header.width + x0) + k] = new_pixel;
                }
            }
        }

        data = new_data;
    }

    void grey() {
        uint32_t channels = bmp_info_header.bit_count / 8;
        uint8_t new_color = 0;

        for (uint32_t x0 = 0; x0 < (uint32_t) bmp_info_header.width; ++x0) {
            for (uint32_t y0 = 0; y0 < (uint32_t) bmp_info_header.height; ++y0) {
                new_color = (
                        data[channels * (y0 * bmp_info_header.width + x0) + 0] +
                        data[channels * (y0 * bmp_info_header.width + x0) + 1] +
                        data[channels * (y0 * bmp_info_header.width + x0) + 2]
                ) / 3;
                for (uint32_t i = 0; i < 3; ++i) {
                    data[channels * (y0 * bmp_info_header.width + x0) + i] = new_color;
                }
            }
        }
    }

    void sobel() {
        // negative();
        uint32_t channels = bmp_info_header.bit_count / 8;
        struct sobel_matrix s_mx;
        int16_t new_pixel_x, new_pixel_y;
        std::vector<uint8_t> new_data = data;

        for (int32_t x0 = 0; x0 < (int32_t) bmp_info_header.width; ++x0) {
            for (int32_t y0 = 0; y0 < (int32_t) bmp_info_header.height; ++y0) {
                for (uint32_t k = 0; k < channels; ++k) {
                new_pixel_x = 0;
                new_pixel_y = 0;
                for (int32_t i = -std::min(s_mx.deviation, y0); i <= 
                                std::min(s_mx.deviation, bmp_info_header.height - y0 - 1); ++i) {
                    for (int32_t j = -std::min(s_mx.deviation, x0); j <= 
                                    std::min(s_mx.deviation, bmp_info_header.width - x0 - 1); ++j) {
                        new_pixel_x += data[channels * ((y0 + i) * bmp_info_header.width + (x0 + j)) + k] * 
                                    s_mx.dataX[s_mx.deviation + i][s_mx.deviation + j];
                        new_pixel_y += data[channels * ((y0 + i) * bmp_info_header.width + (x0 + j)) + k] * 
                                    s_mx.dataY[s_mx.deviation + i][s_mx.deviation + j];
                    }
                }

                new_data[channels * (y0 * bmp_info_header.width + x0) + k] = 
                            (int8_t) std::sqrt(new_pixel_x * new_pixel_x + new_pixel_y * new_pixel_y);
                }
            }
        }

        data = new_data;
    }

    void median_filter(int median_area = 1) {
        uint32_t channels = bmp_info_header.bit_count / 8;
        std::vector<uint8_t> buff;
        int8_t med_pix_count = 0;
        std::vector<uint8_t> new_data = data;
        for (int32_t x = 0; x < (int32_t) bmp_info_header.width; ++x) {
            for (int32_t y = 0; y < (int32_t) bmp_info_header.height; ++y) {
                for (uint32_t k = 0; k < channels; ++k) {
                    buff.clear();
                    med_pix_count = 0;
                    for (int32_t i = -std::min(median_area, x); i <= std::min(median_area, bmp_info_header.width - x - median_area); ++i) {
                        for (int32_t j = -std::min(median_area, y); j <= std::min(median_area, bmp_info_header.height - y - median_area); ++j) {
                            buff.push_back(data[channels * ((y + j) * bmp_info_header.width + (x + i)) + k]);
                            ++med_pix_count;
                        }
                    }
                    std::sort(buff.begin(), buff.end());
                    new_data[channels * (y * bmp_info_header.width + x) + k] = buff[med_pix_count >> 1];
                }
            }
        }

        data = new_data;
    }

    void viniette(double radius = 1.0, double power = 0.8) {
        uint32_t channels = bmp_info_header.bit_count / 8;
        int32_t curr_x = (int32_t) (bmp_info_header.width >> 1);
        int32_t curr_y = (int32_t) (bmp_info_header.height >> 1);
        double max_radius = (std::sqrt(curr_x * curr_x + curr_y * curr_y)) * radius;

        double force = 0;
        for (uint32_t x = 0; x < (uint32_t) bmp_info_header.width; ++x) {
            for (uint32_t y = 0; y < (uint32_t) bmp_info_header.height; ++y) {
                force = viniette_dist(curr_x, curr_y, x, y) / max_radius;
                force *= power;
                force = pow(cos(force), 4);
                for (uint32_t k = 0; k < channels; ++k) {
                    data[channels * (y * bmp_info_header.width + x) + k] *= force;
                }
            }
        }
    }

    void frame(uint32_t x0, uint32_t y0, uint32_t w, uint32_t h) {
        if (x0 + w > (uint32_t) bmp_info_header.width || y0 + h > (uint32_t) bmp_info_header.height) {
            throw std::runtime_error("The region does not fit in the image!");
        }

        uint32_t channels = bmp_info_header.bit_count / 8;
        for (uint32_t y = 0; y < h; ++y) {
            for (uint32_t x = 0; x < w; ++x) {
                for (uint32_t k = 0; k < channels; ++k) {
                    data[channels * (y * w + x) + k] = 
                            data[channels * ((y + y0) * bmp_info_header.width + (x + x0)) + k];
                }
            }
        }
        bmp_info_header.height = h;
        bmp_info_header.width  = w;
    }

    void resize(uint32_t new_width, uint32_t new_height) {
        uint32_t channels = bmp_info_header.bit_count / 8;
        std::vector<uint8_t> new_data(channels * new_width * new_height);

        for (uint32_t x = 0; x < (uint32_t) new_width; ++x) {
            for (uint32_t y = 0; y < (uint32_t) new_height; ++y) {
                for (uint32_t k = 0; k < channels; ++k) {
                    new_data[channels * (y * new_width + x) + k] = 
                    data[channels * (y * bmp_info_header.width * (bmp_info_header.height / new_height) + 
                                    x * (bmp_info_header.width / new_width)) + k];
                }
            }
        }

        data = new_data;
        bmp_info_header.width = new_width;
        bmp_info_header.height = new_height;
    }


private:
    uint32_t row_stride{ 0 };

    struct clarity_matrix {
        int32_t deviation = 1;
        double data[3][3] = {
            {-1, -1, -1},
            {-1,  9, -1},
            {-1, -1, -1}
        };
    };

    struct gauss_matrix {
        int32_t deviation = 2;
        double data[5][5] = {
            {0.000789, 0.006581, 0.013347, 0.006581, 0.00789},
            {0.006581, 0.054901, 0.111345, 0.054901, 0.006581},
            {0.013347, 0.111345, 0.225821, 0.111345, 0.013347},
            {0.006581, 0.054901, 0.111345, 0.054901, 0.006581},
            {0.000789, 0.006581, 0.013347, 0.006581, 0.00789}
        };
    };

    struct sobel_matrix {
        int32_t deviation = 1;
        int32_t dataX[3][3] = {
            {-1,  0,  1},
            {-2,  0,  2},
            {-1,  0,  1}
        };
        int32_t dataY[3][3] = {
            { 1,  2,  1},
            { 0,  0,  0},
            {-1, -2, -1}
        };
    };

    double viniette_dist(int32_t a_x, int32_t a_y, int32_t b_x, int32_t b_y) {
        a_x -= b_x; a_y -= b_y;
        return (double) std::sqrt((double) (a_x * a_x + a_y * a_y));
    }

    void write_headers(std::ofstream &of) {
        of.write((const char*) &file_header, sizeof(file_header));
        of.write((const char*) &bmp_info_header, sizeof(bmp_info_header));
        if (bmp_info_header.bit_count == 32) {
            of.write((const char*) &bmp_color_header, sizeof(bmp_color_header));
        }
    }

    void write_headers_and_data(std::ofstream &of) {
        write_headers(of);
        of.write((const char*) data.data(), data.size());
    }

    uint32_t make_stride_aligned(uint32_t align_stride) {
        uint32_t new_stride = row_stride;
        while (new_stride % align_stride != 0) {
            new_stride++;
        }
        return new_stride;
    }

    void check_color_header(BMPColorHeader &bmp_color_header) {
        BMPColorHeader expected_color_header;
        if(expected_color_header.red_mask != bmp_color_header.red_mask ||
            expected_color_header.blue_mask != bmp_color_header.blue_mask ||
            expected_color_header.green_mask != bmp_color_header.green_mask ||
            expected_color_header.alpha_mask != bmp_color_header.alpha_mask) {
            throw std::runtime_error("Unexpected color mask format! The program expects the pixel data to be in the BGRA format");
        }
        if(expected_color_header.color_space_type != bmp_color_header.color_space_type) {
            throw std::runtime_error("Unexpected color space type! The program expects sRGB values");
        }
    }
};

#endif // BMP_HEADER