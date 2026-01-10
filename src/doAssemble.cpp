#include "Headers.h"

/**
 * 段状态枚举：标记当前扫描行属于哪个区域
 * Global: 初始状态，不允许出现指令或数据
 * Data:   数据段，存储变量、字符串等
 * Text:   代码段，存储指令机器码
 */
enum class SegmentState { Global, Data, Text };

bool handleSegmentDirective(const std::string& input,
                             SegmentState& state,
                             const std::string& path,
                             int line,
                             InstructionList& inst_list,
                             DataList& data_list) {
    // 匹配以 .data 或 .text 开头的行，并可选捕获后面的数值参数
    static const std::regex segment_re(R"(^\s*\.(data|text)\s*(\S+)?)", std::regex::icase);
    std::smatch match;

    if (std::regex_search(input, match, segment_re)) {
        std::string segment_type = toUppercase(match[1].str());
        
        // 更新当前解析器的段状态
        state = (segment_type == "DATA") ? SegmentState::Data : SegmentState::Text;

        // 处理预留空间情况，例如 ".data 100" 表示预留 100 字节空间
        if (match[2].matched && isPositive(match[2].str())) {
            unsigned size_val = toUNumber(match[2].str());
            
            if (state == SegmentState::Data) {
                // 在数据段插入指定长度的零填充
                Data d;
                d.file = path; d.line = line; d.assembly = input; 
                d.address = 0; d.done = true; // 标记已完成，后续Pass不再解析
                d.raw_data.assign(size_val, 0); 
                data_list.push_back(d);
            } else {
                // 指令段必须 4 字节（32位）对齐
                if (size_val % 4 != 0) {
                    throw std::runtime_error("Alignment Error: .text size must be multiple of 4. (" + path + ":" + std::to_string(line) + ")");
                }
                // 在代码段插入指定数量的 NOP (指令机器码 0)
                Instruction inst;
                inst.file = path; inst.line = line; inst.assembly = input; 
                inst.address = 0; inst.done = true;
                inst.machine_code.assign(size_val / 4, 0); 
                inst_list.push_back(inst);
            }
        }
        return true; // 告知主循环此行已作为段定义处理
    }
    return false;
}

/**
 * 汇编器核心函数
 * 执行流程
 * 1. 文本扫描：读取源文件，按段分类存入 List。
 * 2. 第一遍扫描：计算各行地址，填充已知符号（Label）到符号表。
 * 3. 第二遍扫描：解析前向引用（如跳转到后方标签），回填机器码。
 * 4. 输出生成：生成 FPGA 所需的 .coe 镜像文件。
 */
int doAssemble(const std::string &input_path, const std::string &output_dir) {
    // 使用 ifstream 自动管理文件资源 (RAII)
    std::ifstream infile(input_path);
    if (!infile) {
        std::cerr << "Assembler Error: Cannot open input file " << input_path << std::endl;
        return 1;
    }

    InstructionList instruction_list; // 储存得到的指令
    DataList data_list;               // 储存得到的数据
    SegmentState current_state = SegmentState::Global;
    std::string current_line; // 当前处理到的行
    int line_counter = 0;

    // --- 文本预处理与初次分类 ---
    try {
        while (std::getline(infile, current_line)) {
            line_counter++;
            
            // 去除注释和前后空白字符
            std::string clean_line = KillComment(current_line);
            if (clean_line.empty() || clean_line.find_first_not_of(" \t\r\n") == std::string::npos) {
                continue; // 跳过空行
            }

            // 处理 .data / .text 段切换指令
            if (handleSegmentDirective(clean_line, current_state, input_path, line_counter, instruction_list, data_list)) {
                continue;
            }

            // 检查非法行：在定义任何段之前就出现内容
            if (current_state == SegmentState::Global) {
                std::cerr << "Assembler Error: Statement found outside of any segment at " << input_path << ":" << line_counter << std::endl;
                return 1;
            }

            // 根据当前状态将行存入对应的待处理列表
            if (current_state == SegmentState::Data) {
                Data d; d.file = input_path; d.line = line_counter; d.assembly = clean_line;
                data_list.push_back(d);
            } else {
                Instruction inst; inst.file = input_path; inst.line = line_counter; inst.assembly = clean_line;
                instruction_list.push_back(inst);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Critical Error during parsing: " << e.what() << std::endl;
        return 1;
    }
    infile.close();

    // --- 两遍扫描 ---
    SymbolMap symbol_table;           // 存储标签与地址的映射 (Label -> Address)
    UnsolvedSymbolMap unsolved_map;   // 记录那些因为 Label 尚未定义而无法生成的机器码位置

    // Pass 1: 解析数据段。确定变量地址，将数据标签存入符号表。
    if (GeneratedDataSegment(data_list, symbol_table)) {
        std::cerr << "Error in Data Segment Generation." << std::endl;
        return 1;
    }

    // Pass 1: 解析指令段。计算指令地址，尝试编码。
    // 如果遇到跳转指令指向未知的 Label，会先存入 unsolved_map。
    if (GeneratedMachineCode(instruction_list, unsolved_map, symbol_table)) {
        std::cerr << "Error in Machine Code Generation." << std::endl;
        return 1;
    }

    // Pass 2: 符号回填。
    // 此时所有 Label 的地址都已确定，遍历 unsolved_map 并修正之前留空的机器码。
    if (SolveSymbol(unsolved_map, symbol_table)) {
        std::cerr << "Error: Undefined symbols detected." << std::endl;
        return 1;
    }

    // 文件导出
    // 定义一个 Lambda 闭包简化重复的写文件流程
    auto export_to_file = [&](const std::string& filename, auto& list, auto write_func) {
        std::ofstream outfile(output_dir + filename);
        if (outfile) {
            write_func(outfile, list);
            return true;
        }
        std::cerr << "IO Error: Could not write to " << filename << std::endl;
        return false;
    };

    // 生成指令段 COE
    if (!export_to_file("prgmip32.coe", instruction_list, OutputInstruction)) return 1;
    // 生成数据段 COE
    if (!export_to_file("dmem32.coe", data_list, OutputDataSegment)) return 1;
    
    // 生成调试详情文件
    std::ofstream detail_file(output_dir + "details.txt");
    if (detail_file) {
        OutputDetails(instruction_list, data_list, detail_file);
    }

    std::cout << "Assembly completed successfully." << std::endl;
    return 0; 
}