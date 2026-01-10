#include "Headers.h"

/**
 * @brief 处理代码段（Text Segment）
 * * 第一遍扫描的核心逻辑：
 * 1. 遍历指令列表。
 * 2. 提取标签（Label）并建立符号表映射。
 * 3. 调用 DispatchInstruction 解析助记符和操作数，生成初步机器码。
 * 4. 记录未知符号到 unsolved_symbol_map 中，留待后续解析。
 * * @param instruction_list 指令列表（输入/输出）
 * @param unsolved_symbol_map 未解析符号表（输出），用于记录使用了尚未定义标签的指令
 * @param symbol_map 符号表（输出），记录 Label 对应的地址
 * @return true 如果过程中发生错误, false 如果成功
 */
bool AssemblerCore::ProcessTextSegment(InstructionList& instruction_list,
                                       UnsolvedSymbolMap& unsolved_symbol_map,
                                       SymbolMap& symbol_map) {
    current_address = 0; // PC 初始化
    has_error = false;

    for (auto& instruction : instruction_list) {
        // 如果该指令已经被处理过，更新地址并跳过
        if (instruction.done) {
            instruction.address = current_address;
            current_address += 4 * instruction.machine_code.size(); // MIPS 指令为 4 字节
            continue;
        }

        assert(instruction.machine_code.empty());
        current_instruction_ptr = &instruction; // 记录当前指令指针，主要用于 try-catch 块中报错时的上下文定位

        try {
            // 1. 预处理：提取行首的 Label，剥离行尾的注释
            // 返回的 assembly 是去除了 Label 和注释后的纯汇编语句（如 "add $t0, $t1, $t2"）
            std::string assembly = ExtractLabelAndStripComment(current_address, instruction.assembly, symbol_map);
            assembly = toUppercase(assembly); // 统一转大写，实现大小写不敏感

            instruction.address = current_address; // 记录指令的当前 PC 地址

            // 2. 解析指令
            if (!assembly.empty()) {
                // 分发给具体的指令处理函数（R/I/J/Macro），并在内部生成机器码模板
                // DispatchInstruction 内部会执行 current_address += 4
                DispatchInstruction(assembly, instruction, unsolved_symbol_map);
            }
        } catch (const std::exception& e) {
            LogError(e.what(), instruction.assembly);
            has_error = true;
        }
        
        instruction.done = true; // 标记处理完成
        current_instruction_ptr = nullptr;
    }
    return has_error;
}

/**
 * @brief 处理数据段（Data Segment）
 * * 解析 .data 段的伪指令（.byte, .word 等），将数据转换为二进制流并存入内存映像。
 * 同时也处理数据段的 Label。
 * * @param data_list 数据定义列表
 * @param symbol_map 符号表
 * @return true 如果有错, false 成功
 */
bool AssemblerCore::ProcessDataSegment(DataList& data_list, SymbolMap& symbol_map) {
    current_address = 0; // 数据段重新计数
    has_error = false;

    for (auto& data : data_list) {
        if (data.done) {
            data.address = current_address;
            current_address += data.raw_data.size();
            continue;
        }

        assert(data.raw_data.empty());
        
        // 报错统一使用 Instruction* 类型。
        current_instruction_ptr = reinterpret_cast<const Instruction*>(&data); 

        try {
            // 提取 Label (例如: "arr: .word 1, 2, 3")
            std::string assembly = ExtractLabelAndStripComment(current_address, data.assembly, symbol_map);
            assembly = toUppercase(assembly);

            data.address = current_address;

            if (!assembly.empty()) {
                // 解析 .word, .byte 等指令并填充 data.raw_data
                DispatchData(assembly, data);
            }
        } catch (const std::exception& e) {
            LogError(e.what(), data.assembly);
            has_error = true;
        }

        data.done = true;
        current_instruction_ptr = nullptr;
    }
    return has_error;
}

/**
 * @brief 符号重定位/回填（Back-patching）
 * * 在所有代码扫描完成后调用。遍历之前记录的“未解决符号”，
 * 在完整的符号表中查找地址，并填入对应的机器码字段中。
 * * @param unsolved_symbol_map 待解决的符号引用集合
 * @param symbol_map 完整的符号地址表
 * @return true 如果有未定义的符号或解析错误, false 成功
 */
bool AssemblerCore::ResolveSymbols(UnsolvedSymbolMap& unsolved_symbol_map,
                                   const SymbolMap& symbol_map) {
    has_error = false;

    for (const auto& [symbol, references] : unsolved_symbol_map) {
        
        // 检查符号是否存在于全局符号表中
        if (symbol_map.find(symbol) == symbol_map.end()) {
            LogError("Unknown Symbol: " + symbol);
            has_error = true;
            continue; // 发生错误但不影响检查其他符号，继续循环
        }

        int symbol_addr = symbol_map.at(symbol); // 目标符号的绝对地址

        // 遍历所有引用了该符号的指令位置
        for (const auto& ref : references) {
            try {
                current_instruction_ptr = ref.instruction;
                unsigned inst_addr = current_instruction_ptr->address; // 当前指令的地址（PC）
                MachineCode& machine_code = *ref.machine_code_handle;  // 指向机器码的引用

                // 根据指令格式回填不同的字段
                if (isR_Format(machine_code)) {
                    SetShamt(machine_code, symbol_addr);
                } 
                else if (isI_Format(machine_code)) {
                    int imm = symbol_addr;
                    int op = machine_code >> 26; // 提取高6位 Opcode

                    // 分支指令 (beq, bne 等) 使用相对寻址
                    // Offset = (Target Address - (Current PC + 4)) / 4
                    if (IsBranchOpcode(op)) {
                        imm -= (inst_addr + 4); // 减去 (PC + 4)
                        imm >>= 2;              // 除以 4 (右移2位)
                    }
                    // 普通 I-Format (如 lw, addi) 使用绝对地址的低16位或者立即数
                    SetImmediate(machine_code, imm);
                } 
                else if (isJ_Format(machine_code)) {
                    // J-Format (j, jal) 使用伪绝对寻址
                    // Target = Address >> 2 (省略低2位，高4位由 PC 决定，此处仅设置中间26位)
                    SetAddress(machine_code, symbol_addr >> 2);
                } 
                else {
                    throw std::runtime_error("Unknown instruction format during symbol resolution.");
                }

            } catch (const std::exception& e) {
                LogError(e.what(), "Resolving " + symbol);
                has_error = true;
            }
        }
    }
    current_instruction_ptr = nullptr;
    return has_error;
}

/**
 * @brief 指令分发器
 * * 识别助记符类型，调用对应的格式处理函数。
 */
void AssemblerCore::DispatchInstruction(const std::string& assembly, Instruction& instruction,
                                        UnsolvedSymbolMap& unsolved_symbol_map) {
    std::string mnemonic = GetMnemonic(assembly); // 获取第一个单词作为助记符
    MachineCodeIt handle = NewMachineCode(instruction); // 在 instruction 中分配一个新的机器码槽位
    
    // 统一转大写
    std::string upMnemonic = toUppercase(mnemonic);

    // 根据助记符特征分发到具体的解析逻辑
    if (isR_Format(mnemonic)) {
        R_FormatInstruction(upMnemonic, assembly, unsolved_symbol_map, handle, &instruction);
    } else if (isI_Format(mnemonic)) {
        I_FormatInstruction(upMnemonic, assembly, unsolved_symbol_map, handle, &instruction);
    } else if (isJ_Format(mnemonic)) {
        J_FormatInstruction(upMnemonic, assembly, unsolved_symbol_map, handle, &instruction);
    } else if (isMacro_Format(mnemonic)) {
        // 伪指令（Macro）可能展开为多条机器码，需要当前地址上下文
        Macro_FormatInstruction(upMnemonic, assembly, unsolved_symbol_map, handle, current_address, &instruction);
    } else {
        throw UnknownInstruction(mnemonic);
    }

    current_address += 4; // MIPS 指令长度固定为 4 字节
}

/**
 * @brief 解析数据段伪指令 (.byte, .half, .word)
 * * 支持格式示例:
 * .word 10, 20
 * .byte 0xFF:4  (表示值 0xFF 重复 4 次)
 */
void AssemblerCore::DispatchData(const std::string& assembly, Data& data) {
    // 1. 匹配类型：.BYTE / .HALF / .WORD
    // re_type 匹配行首的伪指令类型，Group 1 是类型，Group 2 是后续的数据字符串
    static const std::regex re_type(R"(^\.(BYTE|HALF|WORD)\s+(.+)$)", std::regex::icase);
    
    // 2. 匹配数据 Token：Value : Repeat
    // re_token 匹配 "数值" 或 "数值:重复次数"
    // Group 1: 数值, Group 2: 重复次数 (可选)
    static const std::regex re_token(R"(^([^:,\s]+)\s*(?:\:\s*([^:,\s]+))?(\s*,\s*)?)", std::regex::icase);

    std::cmatch m;
    if (!std::regex_search(assembly.c_str(), m, re_type)) {
        return; // 不是数据定义指令，直接返回
    }

    const std::string type = toUppercase(m[1].str());
    std::string data_stream_str = m[2].str(); // 获取数据部分字符串 (如 "10, 0xFF:2")

    // 循环解析逗号分隔的数据项
    while (!data_stream_str.empty()) {
        std::cmatch token_match;
        if (!std::regex_search(data_stream_str.c_str(), token_match, re_token)) {
            break;
        }

        std::string val_str = token_match[1].str(); // 数值部分
        std::string rep_str = token_match[2].str(); // 重复次数部分（如果有）
        
        // 更新剩余字符串，准备下一次循环解析
        data_stream_str = token_match.suffix().str();

        unsigned repeat_count = 1;
        // 如果存在冒号后的重复次数
        if (token_match[2].matched) {
            if (!isPositive(rep_str)) throw ExceptPositive(rep_str);
            repeat_count = toUNumber(rep_str);
        }

        if (!isNumber(val_str)) throw ExceptNumber(val_str);
        uint32_t val = toNumber(val_str);

        // 根据类型将数据按小端序写入 raw_data
        for (unsigned i = 0; i < repeat_count; i++) {
            if (type == "BYTE") {
                data.raw_data.push_back(val & 0xFF);
                current_address += 1;
            } else if (type == "HALF") {
                data.raw_data.push_back(val & 0xFF);        // 低位
                data.raw_data.push_back((val >> 8) & 0xFF); // 高位
                current_address += 2;
            } else if (type == "WORD") {
                data.raw_data.push_back(val & 0xFF);
                data.raw_data.push_back((val >> 8) & 0xFF);
                data.raw_data.push_back((val >> 16) & 0xFF);
                data.raw_data.push_back((val >> 24) & 0xFF); // 最高位
                current_address += 4;
            } else {
                 throw std::runtime_error("Unknown data directive: " + type);
            }
        }
    }
}

/**
 * @brief 提取 Label 并移除注释
 * * 输入示例: "Loop: add $t1, $t2, $t3 # comment"
 * 动作: 
 * 1. 识别 Label "Loop"，存入 symbol_map，对应当前 address。
 * 2. 移除 "# comment"。
 * 3. 返回 "add $t1, $t2, $t3"。
 * * @param address 当前指令地址
 * @param assembly 原始汇编字符串
 * @param symbol_map 符号表
 * @return std::string 清理后的汇编指令
 */
std::string AssemblerCore::ExtractLabelAndStripComment(unsigned int address, 
                                                       const std::string& assembly, 
                                                       SymbolMap& symbol_map) {
    // 正则解释:
    // \s* : 忽略行首空白
    // (?:(\S+?)\s*:)?  : 可选组。捕获非空字符(\S+)作为Label(Group 1)，后跟冒号
    // \s* : Label后的空白
    // ([^#]*?)         : 捕获指令主体(Group 2)，匹配非 '#' 的字符
    // \s* : 指令后的空白
    // (?:#.*)?         : 可选组。匹配 '#' 开头的注释直到行尾
    static const std::regex re_line(R"(\s*(?:(\S+?)\s*:)?\s*([^#]*?)\s*(?:#.*)?)");
    std::smatch match;

    if (std::regex_match(assembly, match, re_line)) {
        // 如果匹配到了 Label (Group 1)
        if (match[1].matched) {
            std::string label = toUppercase(match[1].str());
            // 查重：不允许重复定义 Label
            if (symbol_map.find(label) != symbol_map.end()) {
                throw std::runtime_error("Redefined symbol: " + label);
            }
            symbol_map[label] = address;
        }
        // 返回指令部分 (Group 2)
        return match[2].str();
    }
    return "";
}

/**
 * @brief 辅助判断 Opcode 是否为 MIPS 分支指令
 * * 用于在符号解析时判断是否需要计算相对偏移量 (PC-Relative)。
 * 涉及: beq(4), bne(5), bgez/bltz(1), bgtz(7), blez(6)
 */
bool AssemblerCore::IsBranchOpcode(int op) const {
    return (op == 0b000100 || op == 0b000101 || // beq, bne
            op == 0b000001 || op == 0b000111 || // bgez/bltz (REGIMM), bgtz
            op == 0b000110);                    // blez
}

/**
 * @brief 错误日志输出
 * @param msg 错误信息
 * @param context 上下文信息（如出错的汇编代码行）
 */
void AssemblerCore::LogError(const std::string& msg, const std::string& context) {
    std::cerr << "[Error] " << msg;
    if (!context.empty()) {
        std::cerr << " | Context: " << context;
    }
    std::cerr << std::endl;
}