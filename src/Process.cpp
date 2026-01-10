#include "Headers.h"

Instruction* cur_instruction = nullptr; // 当前正在处理的指令（用于准确错误定位）
unsigned cur_address = 0;               // 当前地址累加器（每条指令增加 4 字节）

/*
 * 处理 .text 段所有指令的第一遍扫描：
 *  - 处理标签，加入 symbol_map
 *  - 调用 ProcessInstruction 生成机器码
 *  - 更新地址（cur_address）
 *
 * instruction_list 中的每一项 Instruction 会经过一次处理。
 */
int GeneratedMachineCode(InstructionList& instruction_list,
                         UnsolvedSymbolMap& unsolved_symbol_map,
                         SymbolMap& symbol_map) {

    unsigned int address = 0;
    bool meet_error = 0;
    cur_address = 0;

    for (auto& instruction : instruction_list) {

        // 如果这一行已填充
        if (instruction.done) {
            address = cur_address =
                instruction.address + 4 * instruction.machine_code.size();
            continue;
        }

        assert(instruction.machine_code.size() == 0);

        cur_instruction = &instruction;

        // 处理 label，并将汇编命令统一转为大写
        std::string assembly = toUppercase(
            ProcessLabel(address, instruction.assembly, symbol_map));

        instruction.address = address;

        if (assembly != "") {
            try {
                ProcessInstruction(assembly, instruction, unsolved_symbol_map);
            } catch (const std::exception& e) {
                cur_instruction = nullptr;
                meet_error = 1;
            }
            address = cur_address;
        }

        instruction.done = true;
        cur_instruction = nullptr;
    }

    return meet_error;
}

/*
 * ================================
 *   GeneratedDataSegment()
 * ================================
 *
 * 数据段处理：
 *  - 识别标签
 *  - 处理 BYTE/HALF/WORD 伪指令
 *  - 更新地址（每个字节 +1）
 */
int GeneratedDataSegment(DataList& data_list,
                         SymbolMap& symbol_map) {

    unsigned int address = 0;
    bool meet_error = 0;
    cur_address = 0;

    for (auto& data : data_list) {

        if (data.done) {
            address = cur_address = data.address + data.raw_data.size();
            continue;
        }

        assert(data.raw_data.size() == 0);
        cur_instruction = (Instruction*)&data;

        std::string assembly =
            toUppercase(ProcessLabel(address, data.assembly, symbol_map));

        data.address = address;

        if (assembly != "") {
            try {
                ProcessData(assembly, data);
            } catch (const std::exception& e) {
                cur_instruction = nullptr;
                meet_error = 1;
            }

            address = cur_address;
        }

        data.done = true;
    }

    return meet_error;
}

/*
 * ================================
 *   ProcessInstruction()
 * ================================
 *
 * 根据助记符选择 I/R/J/Macro 的处理方式。
 * 每解析一条真实指令 → cur_address += 4。
 */
void ProcessInstruction(const std::string& assembly, Instruction& instruction,
                        UnsolvedSymbolMap& unsolved_symbol_map) {

    if (assembly != "") {

        std::string mnemonic = GetMnemonic(assembly);

        if (isR_Format(mnemonic)) {

            MachineCodeIt handel = NewMachineCode(instruction);
            R_FormatInstruction(toUppercase(mnemonic), assembly,
                                unsolved_symbol_map, handel);
            cur_address += 4;

        } else if (isI_Format(mnemonic)) {

            MachineCodeIt handel = NewMachineCode(instruction);
            I_FormatInstruction(toUppercase(mnemonic), assembly,
                                unsolved_symbol_map, handel);
            cur_address += 4;

        } else if (isJ_Format(mnemonic)) {

            MachineCodeIt handel = NewMachineCode(instruction);
            J_FormatInstruction(toUppercase(mnemonic), assembly,
                                unsolved_symbol_map, handel);
            cur_address += 4;

        } else if (isMacro_Format(mnemonic)) { // 宏指令（mov/push/pop/nop）

            MachineCodeIt handel = NewMachineCode(instruction);
            Macro_FormatInstruction(toUppercase(mnemonic), assembly,
                                    unsolved_symbol_map, handel);
            cur_address += 4;

        } else {
            throw UnkonwInstruction(mnemonic);
        }
    }
}

/*
 * ================================
 *   ProcessData()
 * ================================
 *
 * 处理 BYTE / HALF / WORD 伪指令，支持重复：
 *   .byte 10 : 3    → 生成 0A 0A 0A
 */
void ProcessData(const std::string& assembly, Data& data) {

    if (assembly != "") {

        std::regex re(R"(^\.(BYTE|HALF|WORD)\s+(.+)$)", std::regex::icase);
        std::cmatch m;
        std::regex_search(assembly.c_str(), m, re);

        if (!m.empty()) {
            const std::string type = m[1].str();
            std::string datastr = m[2].str();

            // 匹配 token: value : repeat ,
            std::regex re(R"(^([^:,\s]+)\s*(?:\:\s*([^:,\s]+))?(\s*,\s*)?)",
                          std::regex::icase);

            do {
                std::cmatch m;
                std::regex_search(datastr.c_str(), m, re);

                std::string cur_data_str = m[1].str();
                std::string repeat_time_str = m[2].str();

                datastr = m.suffix();

                unsigned repeat_time = 1;

                if (m[2].matched) {
                    if (isPositive(repeat_time_str))
                        repeat_time = toUNumber(repeat_time_str);
                    else
                        throw ExceptPositive(repeat_time_str);
                }

                if (!isNumber(cur_data_str))
                    throw ExceptNumber(cur_data_str);

                std::uint32_t d = toNumber(cur_data_str);

                // 根据 BYTE / HALF / WORD 写入 raw_data
                for (unsigned i = 0; i < repeat_time; i++) {

                    if (type == "BYTE") {
                        data.raw_data.push_back(d & 0xff);
                        cur_address += 1;

                    } else if (type == "HALF") {
                        data.raw_data.push_back(d & 0xff);
                        data.raw_data.push_back((d >> 8) & 0xff);
                        cur_address += 2;

                    } else if (type == "WORD") {
                        data.raw_data.push_back(d & 0xff);
                        data.raw_data.push_back((d >> 8) & 0xff);
                        data.raw_data.push_back((d >> 16) & 0xff);
                        data.raw_data.push_back(d >> 24);
                        cur_address += 4;

                    } else {
                        throw std::runtime_error("Unkonw error.");
                    }
                }

            } while (datastr != "");

        }
    }
}

/*
 * ================================
 *   ProcessLabel()
 * ================================
 *
 * 提取标签 "Label: instruction"
 * 将标签加入 symbol_map[label] = address
 * 返回 instruction 部分
 */
std::string ProcessLabel(unsigned int address, const std::string& assembly,
                         SymbolMap& symbol_map) {

    static std::regex re("\\s*(?:(\\S+?)\\s*:)?\\s*(.*?)\\s*(?:#.*)?");
    std::smatch match;

    std::string assembly2 = KillComment(assembly);

    std::regex_match(assembly2, match, re);

    if (match[1].matched) {
        std::string label = toUppercase(match[1].str());

        if (symbol_map.find(label) != symbol_map.end())
            throw std::runtime_error("Redefine symbol:" + label);

        symbol_map[label] = address;
    }

    return match[2].str(); // 去掉 label 后的剩余指令
}

/*
 * 去掉注释（# 后的部分）
 */
std::string KillComment(const std::string& assembly) {
    static std::regex re("^([^#]*)(?:#.*)?");
    std::smatch match;
    std::regex_match(assembly, match, re);
    return match[1].str();
}

/*
 * ================================
 *   SolveSymbol()
 * ================================
 *
 * 第二遍扫描：
 *   将所有未解析符号引用位置补全。
 *
 * 对 I/J/R 不同类型字段有不同处理：
 *   - I：立即数
 *   - J：跳转 26-bit 地址（右移 2）
 *   - R：shamt（极少见）
 *
 * 对 branch 指令：
 *   imm = target_addr - (PC+4)
 *   imm >>= 2
 */
int SolveSymbol(UnsolvedSymbolMap& unsolved_symbol_map,
                const SymbolMap& symbol_map) {

    for (auto it = unsolved_symbol_map.begin(); it != unsolved_symbol_map.end(); it++) {

        std::string symbol = it->first;

        for (auto& ref : it->second) {
            try {
                cur_instruction = ref.instruction;
                cur_address = cur_instruction->address;

                if (symbol_map.find(symbol) == symbol_map.end()) {
                    throw std::runtime_error("Unknow Symbol: " + symbol + ".");
                }

                MachineCode& machine_code = *ref.machine_code_handle;

                // R-format: 填 shamt（一般用于测试）
                if (isR_Format(machine_code)) {
                    int shamt = symbol_map.at(symbol);
                    SetShamt(machine_code, shamt);
                }

                // I-format
                else if (isI_Format(machine_code)) {

                    int imm = symbol_map.at(symbol);
                    int op = machine_code >> 26;

                    // 分支跳转偏移计算
                    if (op == 0b000100 || op == 0b000101 ||
                        op == 0b000001 || op == 0b000111 ||
                        op == 0b000110) {
                        imm -= cur_address + 4;
                        imm >>= 2;
                    }

                    SetImmediate(machine_code, imm);
                }

                // J-format: 地址右移两位
                else if (isJ_Format(machine_code)) {
                    int addr = symbol_map.at(symbol) >> 2;
                    SetAddress(machine_code, addr);
                }

                else
                    throw std::runtime_error("Unknow errors.");

                cur_instruction = nullptr;
            }

            catch (const std::exception& e) {

                if (symbol_map.find(symbol) != symbol_map.end()) {
                    std::cout<<"This error occurs while solving symbol. At this time, " + symbol + " = " + std::to_string(symbol_map.at(symbol));
                }

                cur_instruction = nullptr;
                return 1;
            }
        }
    }

    return 0;
}