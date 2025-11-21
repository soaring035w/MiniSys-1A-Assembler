#include "Headers.h"

/*
 * doAssemble：汇编器主流程
 *
 * 主要步骤：
 *  1. 打开输入文件并逐行读取
 *  2. 识别 .data / .text 段
 *  3. 将数据段行保存到 DataList，将指令行保存到 InstructionList
 *  4. 执行两遍扫描：
 *      一、生成数据段（处理符号、计算偏移）
 *      二、生成文本段机器码（指令编码）
 *      三、解决延迟符号（反向修正跳转等）
 *  5. 输出到 .coe 文件
 */
int doAssemble(const std::string &input_file_path,
               const std::string &output_folder_path) {
    bool meet_error = 0;  // 是否发生错误

    std::fstream file;
    file.open(input_file_path, std::ios_base::in);
    if (!file.is_open()) {
        std::cout<< "fail to open file: " + input_file_path;
        return 1;
    }

    InstructionList instruction_list;  // 存储 .text 段中的每条指令
    DataList data_list;                // 存储 .data 段中的每条数据

    int line = 0;  // 当前行号

    // 枚举段类型
    enum { global, data, text };
    int state = global;  // 初始状态，没有进入任何段

    // ========== 逐行读取源文件 ==========
    while (!file.eof()) {
        line++;
        std::string input;
        getline(file, input);
        input = KillComment(input);  // 去掉注释（; # // 等）

        /*
         * 使用正则匹配：
         *   .data
         *   .text
         *   .data 100
         *   .text 20
         *
         * 其中 m[1] 为段名（data 或 text）
         *      m[2] 为可选的数值（初始化填充大小）
         */
        std::regex re(R"(^\s*\.(data|text)\s*(\S+)?)", std::regex::icase);
        std::cmatch m;
        std::regex_search(input.c_str(), m, re);

        // ---------- 匹配到段声明 ----------
        if (!m.empty()) {
            if (toUppercase(m[1].str()) == "DATA") {
                state = data;

                // 如果 .data 后跟数字，如 `.data 16`
                if (m[2].matched) {
                    if (isPositive(m[2].str())) {
                        unsigned pos = toUNumber(m[2].str());

                        // 创建一条空 Data（填充 pos 字节的 0）
                        Data data;
                        data.file = input_file_path;
                        data.line = line;
                        data.assembly = input;

                        // 初始化空空间
                        for (unsigned i = 0; i < pos; i++) {
                            data.raw_data.push_back(0);
                        }
                        data.address = 0;
                        data.done = true;  // 已经生成完（无须再解析）
                        data_list.push_back(data);

                    }
                }

            } else {  // ---------- 处理 .text ----------
                state = text;

                if (m[2].matched) {
                    if (isPositive(m[2].str())) {
                        unsigned pos = toUNumber(m[2].str());

                        // text 段必须 4 字节对齐
                        if (pos % 4 != 0) {
                            std::cerr<< "DWORD-aligned error." + input_file_path + '(' + std::to_string(line) +')';
                            return 1;
                        }

                        // 创建空指令（填充 NOP）
                        Instruction instruction;
                        instruction.file = input_file_path;
                        instruction.line = line;
                        instruction.assembly = input;

                        // pos / 4 个 NOP
                        for (unsigned i = 0; i < pos / 4; i++) {
                            instruction.machine_code.push_back(0);
                        }
                        instruction.address = 0;
                        instruction.done = true;
                        instruction_list.push_back(instruction);

                    }
                }
            }

            continue;  // 本行处理段定义结束
        }

        // ---------- 未进入任何段 ----------
        if (state == global) {
            std::cerr<<"Need a segment." + input_file_path + '(' + std::to_string(line) + ')';
            return 1;

        // ---------- 文本段 ----------
        } else if (state == text) {
            Instruction instruction;
            instruction.file = input_file_path;
            instruction.line = line;
            instruction.assembly = input;
            instruction_list.push_back(instruction);

        // ---------- 数据段 ----------
        } else if (state == data) {
            Data data;
            data.file = input_file_path;
            data.line = line;
            data.assembly = input;
            data_list.push_back(data);
        }
    }

    file.close();

    // 开始第二阶段：两遍扫描

    UnsolvedSymbolMap unsolved_symbol_map; // 存放第一次扫描后无法确定的符号（如前向引用）
    SymbolMap symbol_map;                  // 存放已解析出的符号表（标签 -> 地址）

    /*
     * 第一遍扫描数据段：
     *   解析每条 Data（.word, .byte, .string, 标签等）
     *   计算每条数据的地址
     *   收集符号（标签）
     *   遇到无法确定的符号位置 → 放入 unsolved_symbol_map
     */
    if (GeneratedDataSegment(data_list, symbol_map)) {
        meet_error = 1;
    }

    /*
     * 第一遍扫描文本段（指令）：
     *   解析每条 Instruction 的助记符/寄存器/立即数
     *   将其编码成机器码（若部分立即数为符号 → 标记未解决符号）
     *   同样填充 instruction_list.address
     */
    if (GeneratedMachineCode(instruction_list, unsolved_symbol_map,
                             symbol_map)) {
        meet_error = 1;
    }

    /*
     * 第二遍扫描：解决所有符号
     *
     * 这一步负责：
     *   - 将 unsolved_symbol_map 中所有引用符号的位置反向填入正确地址
     *   - 支持前向引用（跳转到后面才声明的 Label）
     *
     * 如果有符号最终仍找不到 → 报错 undefined symbol
     */
    if (!meet_error && SolveSymbol(unsolved_symbol_map, symbol_map)) {
        meet_error = 1;
    }

    
    // 所有内容生成完毕，开始输出
    if (!meet_error) {
        // 输出指令段到 prgmip32.coe
        file.open(output_folder_path + "prgmip32.coe", std::ios_base::out);
        if (file.is_open()) {
            /*
             * OutputInstruction：
             *   将 instruction_list 中生成的机器码按 COE 文件格式输出
             *   用于 FPGA Block RAM 初始化
             */
            OutputInstruction(file, instruction_list);
        } else {
            std::cerr<<"fail to open file: " + output_folder_path + "prgmip32.coe";
        }
        file.close();

        
        // 输出数据段到 dmem32.coe
        file.open(output_folder_path + "dmem32.coe", std::ios_base::out);
        if (file.is_open()) {
            /*
             * OutputDataSegment：
             *   将 data_list 中的 raw_data 输出成 .coe 格式
             *   用于 FPGA 数据存储初始化
             */
            OutputDataSegment(file, data_list);
        } else {
            std::cerr<<"fail to open file: " + output_folder_path + "dmem32.coe";
        }
        file.close();

        // 打印每条指令和数据的解析结果到 details.txt
        file.open(output_folder_path + "details.txt", std::ios_base::out);
            if (file.is_open()) {
                /*
                 * ShowDetails：
                 *   
                 *   包括地址、编码、原语句等
                 */
                OutputDetails(instruction_list, data_list, file);
            } else {
                std::cerr<<"fail to open file: " + output_folder_path + "details.txt";
            }
        file.close();
    }

    return meet_error;  // 0 = 成功，1 = 出现错误
}