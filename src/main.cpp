#include "Headers.h"

int main(int argc, char* argv[]) {
    // 程序名之外必须是 1 个或 2 个参数
    if (argc != 2 && argc != 3) {
        std::cerr << "Error: Invalid input.\n";
        std::cerr << "Usage:\n"
                  << "  mas.exe input_file_path\n"
                  << "  mas.exe input_file_path output_folder_path\n";
        return 1;
    }

    std::string input_path = argv[1];
    std::string output_folder;

    // 两个参数：指定了输出路径
    if (argc == 3) {
        output_folder = argv[2];
    }

    // 调用汇编处理函数
    try {
        doAssemble(input_path, output_folder);
    } catch (const std::exception& e) {
        std::cerr << "Assemble failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}