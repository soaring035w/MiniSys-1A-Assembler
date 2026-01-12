#include "Headers.h"

/*
 * R 格式指令匹配正则：
 *
 * 包含：
 *   - 加法：add addu sub subu
 *   - 逻辑：and or xor nor
 *   - 比较：slt sltu
 *   - 移位：sll srl sra sllv srlv srav
 *   - 跳转：jr jalr
 *   - 乘除：mult multu div divu
 *   - 系统：break syscall eret
 *   - HI/LO 相关：mfhi mflo mthi mtlo
 *
 */
std::regex R_format_regex(
    "^(addu?|subu?|and|[xn]?or|sltu?|s(?:ll|rl|ra)v?|jr|multu?|divu?|m[tf]hi|"
    "m[tf]lo|jalr|break|syscall|eret)",
    std::regex::icase);

/*
 * R_FormatInstruction：
 *
 * 处理三种常见 R 格式编码：
 *   1. 三寄存器：op rd, rs, rt
 *   2. 两寄存器：op rs, rt
 *   3. 单寄存器：op rs
 *   4. 无操作数：break / syscall / eret
 *
 * R 格式结构：
 * 31-26 | 25-21 | 20-16 | 15-11 | 10-6 | 5-0
 *   OP  |  RS   |  RT   |  RD   |Shamt | Func
 *
 * OP 恒为 0（ERET 特例会被改成 0x10）
 */
MachineCode R_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeIt machine_code_it,
                                Instruction* cur_instruction) {

    MachineCode& machine_code = *machine_code_it;
    machine_code = 0;

    // 提取三个操作数
    std::string op1, op2, op3;
    GetOperand(assembly, op1, op2, op3);

    // R 型中OP 字段 = 0（除 ERET、MFC0、MTC0）
    SetOP(machine_code, 0);

    
    // 一、 三操作数 R 指令（ADD/ADDU/SUB/SUBU/SLT/移位变量类）
    //    形式： op rd, rs, rt
    
    if (!op1.empty() && !op2.empty() && !op3.empty()) {

        // 常规算术与逻辑类，Func 对应如下
        std::unordered_map<std::string, int> func1{
            {"ADD", 0b100000},  {"ADDU", 0b100001}, {"SUB", 0b100010},
            {"SUBU", 0b100011}, {"AND", 0b100100}, {"OR", 0b100101},
            {"XOR", 0b100110},  {"NOR", 0b100111}, {"SLT", 0b101010},
            {"SLTU", 0b101011}, {"SLLV", 0b000100},{"SRLV",0b000110},
            {"SRAV",0b000111}
        };

        // 固定移位（shift immediate）
        std::unordered_map<std::string, int> func2{
            {"SLL", 0b000000}, {"SRL", 0b000010}, {"SRA", 0b000011}
        };

        auto it1 = func1.find(mnemonic);
        auto it2 = func2.find(mnemonic);

        /*
         *   三寄存器常规算术/逻辑类
         *
         *   rd = op1
         *   rs = op2
         *   rt = op3       ← 对于SLLV/SRLV/SRAV，需要交换 op2/op3
         */
        if (it1 != func1.end()) {

            // 变量移位(sllv/srlv/srav)参数顺序与标准 R 格式不同，需交换
            if (mnemonic == "SLLV" || mnemonic == "SRLV" || mnemonic == "SRAV") {
                std::swap(op2, op3);
            }

            SetFunc(machine_code, func1.at(mnemonic));
            SetRS(machine_code, Register(op2));
            SetRT(machine_code, Register(op3));
            SetRD(machine_code, Register(op1));
            SetShamt(machine_code, 0);
        }

        /*
         * 固定移位：sll / srl / sra
         *
         * 格式：
         *   sll rd, rt, shamt
         */
        else if (it2 != func2.end() && (isNumber(op3) || isSymbol(op3))) {
            
            SetFunc(machine_code, func2.at(mnemonic));
            SetRS(machine_code, 0);
            SetRT(machine_code, Register(op2));
            SetRD(machine_code, Register(op1));

            if (isNumber(op3)) {
                SetShamt(machine_code, toNumber(op3));
            } else {
                // shamt 使用符号 → 第一次扫描先占位
                SetShamt(machine_code, 0);
                unsolved_symbol_map[op3].push_back(
                    SymbolRef{machine_code_it, cur_instruction});
            }

        } else goto err;
    }

    
    // 二、 二操作数 R 指令：MULT/MULTU/DIV/DIVU/JALR
    
    else if (!op1.empty() && !op2.empty() && op3.empty()) {

        std::unordered_map<std::string, int> func3{
            {"MULT", 0b011000},
            {"MULTU",0b011001},
            {"DIV",  0b011010},
            {"DIVU", 0b011011},
            {"JALR", 0b0001001}
        };

        auto it3 = func3.find(mnemonic);

        if (it3 != func3.end()) {

            // MULT/MULTU/DIV/DIVU
            if (mnemonic != "JALR") {
                SetRS(machine_code, Register(op1));
                SetRT(machine_code, Register(op2));
                SetRD(machine_code, 0);
            } 
            // JALR rd, rs
            else {
                SetRS(machine_code, Register(op2));
                SetRT(machine_code, 0);
                SetRD(machine_code, Register(op1));
            }

            SetShamt(machine_code, 0);
            SetFunc(machine_code, func3.at(mnemonic));

        } else goto err;
    }

    
    // 单操作数 R 指令：JR/MFHI/MFLO/MTHI/MTLO
    
    else if (!op1.empty() && op2.empty() && op3.empty()) {

        std::unordered_map<std::string, int> func4{
            {"JR",   0b001000},
            {"MFHI", 0b010000},
            {"MFLO", 0b010010},
            {"MTHI", 0b010001},
            {"MTLO", 0b010011}
        };

        auto it4 = func4.find(mnemonic);

        if (it4 != func4.end()) {

            // JR rs
            if (mnemonic != "MFHI" && mnemonic != "MFLO") {
                SetRS(machine_code, Register(op1));
                SetRT(machine_code, 0);
                SetRD(machine_code, 0);
            }
            // MFHI rd   / MFLO rd
            else {
                SetRS(machine_code, 0);
                SetRT(machine_code, 0);
                SetRD(machine_code, Register(op1));
            }

            SetShamt(machine_code, 0);
            SetFunc(machine_code, func4.at(mnemonic));

        } else goto err;
    }

    
    // 无操作数指令：BREAK / SYSCALL / ERET
    
    else if (op1.empty() && op2.empty() && op3.empty()) {

        std::unordered_map<std::string, int> func5{
            {"BREAK", 0b001101},
            {"SYSCALL",0b001100},
            {"ERET",   0b011000}
        };

        auto it5 = func5.find(mnemonic);

        if (it5 != func5.end()) {

            // ERET 特例：OP = 0x10, RS = 0x10
            if (mnemonic == "ERET") {
                SetOP(machine_code, 0b010000);
            }

            SetRS(machine_code, mnemonic == "ERET" ? 0b10000 : 0);
            SetRT(machine_code, 0);
            SetRD(machine_code, 0);
            SetShamt(machine_code, 0);
            SetFunc(machine_code, func5.at(mnemonic));

        } else goto err;
    }

    
    // 其他情况均视为错误
    
    else {
    err:
        if (isR_Format(assembly))
            throw OperandError(mnemonic);
        else
            throw UnknownInstruction(mnemonic);
    }

    return machine_code;
}

/*
 * 判断机器码是否为 R 格式：
 *   R 格式要求 OP == 0
 */
bool isR_Format(MachineCode machine_code) {
    return (machine_code >> 26 == 0 || machine_code >> 26 == 0b010000); // 包含 ERET 特例
}

/*
 * 判断汇编行是否为 R 格式：
 *   使用正则匹配指令助记符
 */
bool isR_Format(const std::string& assembly) {
    std::string mnemonic = GetMnemonic(assembly);
    std::cmatch m;
    std::regex_match(mnemonic.c_str(), m, R_format_regex);

    // 必须完全匹配助记符
    return (!m.empty() && m.prefix().str().empty() && m.suffix().str().empty());
}