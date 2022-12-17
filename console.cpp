#include <iostream>
#include <sstream>
#include <string>
#include <boost/filesystem.hpp>
#include "BMP.h"


const char hello_msg[] =
"-------/ BMP Redactor /-------\n"
"---/ Created by Lev Bunin /---\n\n"
"Try `help` for more information!\n"
;

const char help_msg[] =
"-----/ BMP Redactor Helper /-----\n\n"
"`exit`\n"
"\tClosing the terminal and exit from program.\n\n"
"`help`\n"
"\tPrinting commands and flags.\n\n"
"`ls [-flags ...]`\n"
"\tStandard ls command with flag support.\n\n"
"`cd [-flags ...]`\n"
"\tStandard cd command with flag support.\n\n"
"`mkdir [-flags ...]`\n"
"\tStandard mkdir command with flag support.\n\n"
"`rm [-flags ...]`\n"
"\tStandard rm command with flag support.\n\n"
"`open [/.../path_to.bmp]`\n"
"\tOpening .bmp file for changing and/or writing.\n\n"
"`write [/.../path_to_save.bmp]`\n"
"\tSaving .bmp file.\n\n"
"`change [options]`\n"
"\tChanging file by using flags:\n\n"
"\t\"-negative\" / \"-n\"\n"
"\t\tNegative filter.\n\n"
"\t\"-replace-color\" / \"-rc\"\n"
"\t\tReplace RGB(1) color -> RGB(2).\n"
"\t\t* Sends a request (stdin) about getting color parameters.\n\n"
"\t\"-clarity\" / \"-cl\"\n"
"\t\tClarity filter\n"
"\t\t* Sends a request (stdin) about getting ~clarity force~ parameter\n\n"
"\t\"-gauss\"\n"
"\t\tGauss filter.\n\n"
"\t\"-grey\" / \"-g\"\n"
"\t\tGrey filter.\n\n"
"\t\"-sobel\" / \"-s\"\n"
"\t\tBorder selection filter.\n\n"
"\t\"-median\" / \"-m\"\n"
"\t\tMedian filter.\n"
"\t\t* Sends a request (stdin) about getting ~median area~ parameter.\n\n"
"\t\"-viniette\" / \"-v\"\n"
"\t\tViniette filter.\n"
"\t\t* Sends a request (stdin) about getting ~radius & power~ parameters\n\n"
"\t\"-frame\" / \"-f\"\n"
"\t\tFraming .bmp\n"
"\t\t* Sends a request (stdin) about getting ~x0, y0, w, h~ parameters\n\n"
"\t\"-resize\" / \"-rs\"\n"
"\t\tResizing picture and changing width, height\n"
"\t\t* Sends a request (stdin) about getting ~new_w, new_h~ parameters\n\n"
"-----\\ BMP Redactor Helper \\-----\n"
;

using namespace boost::filesystem;

void open_console() {
    bool is_need_exit = false;
    bool is_request_ok = false;
    bool is_bmp_opened = false;
    std::string request_conf;
    std::string comm;
    std::string other_comm;
    std::string bmp_path;
    BMP bmp;

    std::cout << hello_msg;
    while (!is_need_exit) {
        std::cout << current_path().c_str() << "$ ";

        std::cin >> comm;
        if (comm == "exit") {
            is_need_exit = true;
        } else 
        if (comm == "help") {
            std::cout << help_msg;
        } else 
        if (comm == "ls") {
            std::getline(std::cin, other_comm);
            if (other_comm.empty()) {
                std::cout << "Error in ls options!\n";
                continue;
            }
            try {
                system(("ls " + other_comm).c_str());
            }
            catch(...) {
                std::cout << "Error in ls command!\n";
            }
        } else 
        if (comm == "cd") {
            std::cin >> other_comm;
            if (other_comm.empty()) {
                std::cout << "Error in cd options!\n";
                continue;
            }
            try {
                current_path(other_comm);
            }
            catch(...) {
                std::cout << "Error in cd command!\n";
            }
        } else 
        if (comm == "mkdir") {
            std::cin >> other_comm;
            if (other_comm.empty()) {
                std::cout << "Error in mkdir options!\n";
                continue;
            }
            try {
                create_directory(other_comm);
            }
            catch(...) {
                std::cout << "Error in mkdir command!\n";
            }
        } else 
        if (comm == "rm") {
            std::cin >> other_comm;
            if (other_comm.empty()) {
                std::cout << "Error in rm options!\n";
                continue;
            }
            try {
                boost::filesystem::remove(other_comm);
            }
            catch(...) {
                std::cout << "Error in rm command!\n";
            }
        } else 
        if (comm == "open") {
            std::cin >> bmp_path;
            bmp.read(bmp_path.c_str());
            std::cout << '"' << bmp_path << "\" opened!\n";
            is_bmp_opened = true;
        } else 
        if (comm == "write") {
            std::cin >> bmp_path;
            bmp.write(bmp_path.c_str());
            std::cout << '"' << bmp_path << "\" wrote!\n";
        } else 
        if (comm == "change") {
            std::getline(std::cin, other_comm);

            if (!is_bmp_opened) {
                std::cout << "There is no opened .bmp files. Use `open` command to open .bmp\n";
                std::cout << "Try help for more information!\n";
                continue;
            }

            std::istringstream to_split(other_comm);

            for (std::string optn; to_split >> optn && !optn.empty();) {
                if (optn == "-negative" || optn == "-n") {
                    std::cout << "Setting negative to \"" << bmp_path << "\"...\n";
                    bmp.negative();
                } else 
                if (optn == "-replace-color" || optn == "-rc") {
                    uint32_t R1, G1, B1, A1, R2, G2, B2, A2 = 1;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter (R1, G1, B1)->(R2, G2, B2) colors in range [0, 255] splitted by space to replace color in \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> R1 >> G1 >> B1 >> R2 >> G2 >> B2;
                        std::cout << "Replacing color (" << R1 << "; " << G1 << "; " << B1 << ")->(" 
                                    << R2 << "; " << G2 << "; " << B2 << ") in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }

                    std::cout << bmp.replace_color(R1, G1, B1, A1, R2, G2, B2, A2) 
                            << " pixels have changed!\n";
                } else 
                if (optn == "-clarity" || optn == "-cl") {
                    double clarity_force = 8;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter ~clarity force~ (double) to set clarity filter in \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> clarity_force;
                        std::cout << "Setting clarity filter with force = " << clarity_force 
                                        << " in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }

                    if (clarity_force == 0.0) {
                        bmp.clarity();
                    } else {
                        bmp.clarity(clarity_force);
                    }
                } else 
                if (optn == "-gauss") {
                    std::cout << "Setting gauss filter in \"" 
                                    << bmp_path << "\"...\n";
                    bmp.gauss();
                } else 
                if (optn == "-grey" || optn == "-g") {
                    std::cout << "Setting grey filter in \"" 
                                        << bmp_path << "\"...\n";
                    bmp.grey();
                } else 
                if (optn == "-sobel" || optn == "-s") {
                    std::cout << "Setting border selection filter in \"" 
                                        << bmp_path << "\"...\n";
                    bmp.sobel();
                } else 
                if (optn == "-median" || optn == "-m") {
                    int median_area = 1;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter ~median area~ (int) to set median filter in \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> median_area;
                        std::cout << "Setting median filter with median area = " << median_area 
                                        << " in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }
                    if (median_area == 0) {
                        bmp.median_filter();
                    } else {
                        bmp.median_filter(median_area);
                    }
                } else 
                if (optn == "-viniette" || optn == "-v") {
                    double radius = 1.0, power = 0.8;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter ~radius, power~ (double) to set viniette filter in \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> radius >> power;
                        std::cout << "Setting viniette filter with (radius; power) = (" << radius 
                                        << "; " << power << ") in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }

                    bmp.viniette(radius, power);
                } else 
                if (optn == "-frame" || optn == "-f") {
                    uint32_t x0, y0, w, h;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter ~x0, y0, w, h~ (uint) to frame \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> x0 >> y0 >> w >> h;
                        std::cout << "Framing with (x0; y0; w; h) = (" << x0 << "; " << y0 << "; " 
                                        << w << "; " << h << ") in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }

                    bmp.frame(x0, y0, w, h);
                } else 
                if (optn == "-resize" || optn == "-rs") {
                    uint32_t new_width, new_height;

                    is_request_ok = false;
                    while (!is_request_ok) {
                        std::cout << "Please, enter ~w, h~ (uint) to resize \"" 
                                        << bmp_path << "\"...\n";
                        std::cin >> new_width >> new_height;
                        std::cout << "Resizing with (w; h) = (" << new_width << "; " 
                                        << new_height << ") in \"" << bmp_path << "\"...\n";
                        
                        std::cout << "\nAre you sure? (y/n): ";
                        std::cin >> request_conf;
                        if (request_conf == "y") {
                            is_request_ok = true;
                        }
                    }

                    bmp.resize(new_width, new_height);
                }
                else {
                    std::cout << "Wrong option: `" << optn << "`!\n";
                    std::cout << "Try `help` for more information!\n";
                    continue;
                }
            }
        }
    }
}
