#include "Headers.h"

/*
 * I 格式指令识别用的正则表达式：
 *
 * 包含以下种类：
 *   - 加减立即数：addi/addiu
 *   - 按位：andi / ori / xori
 *   - 加载立即数：lui
 *   - 加载存储：lb lh lw lbu lhu sb sh sw
 *   - 分支：beq bne bltz bgez bltzal bgezal bgtz blez
 *   - Coprocessor0：mfc0 mtc0
 */
std::regex I_format_regex(
    "^(addiu?|andi|x?ori|lui|l[bhw]u?|s[bhw]|beq|bne|sltiu?|m[ft]c0|b[gl][et]z|"
    "bgezal|bltzal)",
    std::regex::icase);

/*
 * I_FormatInstruction
 *
 * 参数：
 *   mnemonic：助记符
 *   assembly：完整汇编语句
 *   unsolved_symbol_map：未解决符号表（用于回填）
 *   machine_code_it：当前指令 machine_code 的迭代器
 *
 * 主要处理四类指令：
 *   1. COP0（MFC0 / MTC0）
 *   2. load/store 形式（LW rt, offset(rs)）
 *   3. 普通三操作数 I 指令（ADDI/ORI/ANDI/...）
 *   4. 特殊二操作数指令（LUI、分支跳转组）
 */
MachineCode I_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeIt machine_code_it,
                                Instruction* cur_instruction) {

    MachineCode& machine_code = *machine_code_it;
    machine_code = 0;  // 初始化

    // 解析三个操作数
    std::string op1, op2, op3;
    GetOperand(assembly, op1, op2, op3);

    // 检测是否为内存读写格式，如 lw rt, offset(rs)
    bool is_mem = std::regex_match(mnemonic, std::regex("^L[BHW]U?|S[BHW]$"));

    
    // COP0 指令（MFC0 / MTC0）
    if (mnemonic == "MFC0" || mnemonic == "MTC0") {
        /*
         * COP0 其实属于 R 格式，但为了方便，把它当成 I 格式统一处理。
         *
         * 格式：
         *    MFC0 rt, rd, sel
         * sel 如未指定，则置 0 并给出提示
         */
        if (op3.empty()) {
            op3 = "0";
            std::cout<< "Unset sel field, set it to 0.";
        }

        unsigned sel = toUNumber(op3);
        if (sel > 7) throw NumberOverflow("Sel", "7", std::to_string(sel));

        SetOP(machine_code, 0b010000);                      // COP0
        SetRS(machine_code, mnemonic == "MFC0" ? 0 : 0b00100); // MTC0 使用 RS = 4
        SetRT(machine_code, Register(op1));
        SetRD(machine_code, Register(op2));
        SetShamt(machine_code, 0);
        SetFunc(machine_code, sel);

    }

    
    // 二、 内存访问指令（LW/SW/LH/...）
    // 格式： op rt, offset(rs)
    
    else if (is_mem) {

        static std::regex re(
            "\\s*\\S+\\s*(\\S+)\\s*,\\s*(\\S+)\\s*\\(\\s*(\\S+)\\)",
            std::regex::icase);

        std::smatch match;
        std::regex_match(assembly, match, re);

        if (!match.empty()) {

            // match[1] = rt
            // match[2] = offset
            // match[3] = rs
            std::string op1 = match[1].str(), offset = match[2].str(),
                        op2 = match[3].str();

            // offset 可以是数字或符号
            if (isNumber(offset) || isSymbol(offset)) {

                // 根据不同的 load/store 设置 OP
                if (mnemonic == "LW")      SetOP(machine_code, 0b100011);
                else if (mnemonic == "LH") SetOP(machine_code, 0b100001);
                else if (mnemonic == "LHU")SetOP(machine_code, 0b100101);
                else if (mnemonic == "LB") SetOP(machine_code, 0b100000);
                else if (mnemonic == "LBU")SetOP(machine_code, 0b100100);
                else if (mnemonic == "SW") SetOP(machine_code, 0b101011);
                else if (mnemonic == "SH") SetOP(machine_code, 0b101001);
                else if (mnemonic == "SB") SetOP(machine_code, 0b101000);
                else goto err;

                // rs = 基址寄存器
                SetRS(machine_code, Register(op2));
                // rt = 目标寄存器
                SetRT(machine_code, Register(op1));

                // offset 立即数
                if (isNumber(offset)) {
                    SetImmediate(machine_code, toNumber(offset));
                } else {
                    // 符号，先用 0 占位，加入未解决符号表
                    SetImmediate(machine_code, 0);
                    unsolved_symbol_map[offset].push_back(
                        SymbolRef{machine_code_it, cur_instruction});
                }
            } else {
                throw ExceptNumberOrSymbol(offset);
            }

        } else {
            if (isI_Format(assembly)) throw OperandError(mnemonic);
            else throw UnknownInstruction(mnemonic);
        }
    }

    
    // 普通三操作数 I 指令：ADDI/ORI/SLTI/BEQ/BNE 等
    //    形式：op rt, rs, imm
    
    else {
        if (!op1.empty() && !op2.empty() && !op3.empty()) {

            std::unordered_map<std::string, int> op{
                {"ADDI", 0b001000}, {"ADDIU", 0b001001},
                {"ANDI", 0b001100}, {"ORI", 0b001101},
                {"XORI", 0b001110}, {"BEQ", 0b000100},
                {"BNE", 0b000101},  {"SLTI", 0b001010},
                {"SLTIU", 0b001011}
            };

            auto it = op.find(mnemonic);
            if (it != op.end()) {

                // BEQ/BNE 操作数顺序不同，需要交换
                if (mnemonic == "BEQ" || mnemonic == "BNE") {
                    std::swap(op1, op2);
                }

                SetOP(machine_code, it->second);
                SetRS(machine_code, Register(op2));
                SetRT(machine_code, Register(op1));

                // imm 可能是数字或符号
                if (isNumber(op3)) {
                    SetImmediate(machine_code, toNumber(op3));

                    if (mnemonic.front() == 'B') {
                        std::cout<<("Immediate value in branch instruction.\n");
                    }

                // 如果是符号，需要加入未解决符号表
                } else if (isSymbol(op3)) {
                    SetImmediate(machine_code, 0);
                    unsolved_symbol_map[op3].push_back(
                        SymbolRef{machine_code_it, cur_instruction});
                } else {
                    throw ExceptNumberOrSymbol(op3);
                }
            }
        }

        // 二操作数 I 指令：LUI、BGEZ、BLTZ、BGEZAL、BLTZAL
        else if (!op1.empty() && !op2.empty() && op3.empty()) {

            std::unordered_map<std::string, int> op{
                {"LUI", 0b001111}, {"BGEZ", 0b000001},
                {"BGTZ",0b000111}, {"BLEZ", 0b000110},
                {"BLTZ",0b000001}, {"BGEZAL",0b000001},
                {"BLTZAL",0b000001}
            };

            std::unordered_map<std::string, int> rt{
                {"BGEZ", 1}, {"BGTZ", 0},
                {"BLEZ", 0}, {"BLTZ", 0},
                {"BGEZAL",0b10001}, {"BLTZAL",0b10000}
            };

            SetOP(machine_code, op.at(mnemonic));

            if (mnemonic == "LUI") {
                SetRS(machine_code, 0);
                SetRT(machine_code, Register(op1));
            } else {
                SetRS(machine_code, Register(op1));
                SetRT(machine_code, rt.at(mnemonic));
            }

            // 立即数或符号的处理
            if (isNumber(op2)) {
                SetImmediate(machine_code, toNumber(op2));

                if (mnemonic[0] == 'B')
                    std::cout<< "Immediate value in branch instruction.\n";

            } else if (isSymbol(op2)) {

                SetImmediate(machine_code, 0);
                unsolved_symbol_map[op2].push_back(
                    SymbolRef{machine_code_it, cur_instruction});

            } else throw ExceptNumberOrSymbol(op2);

        }

        else {
        err:
            if (isI_Format(assembly)) throw OperandError(mnemonic);
            else throw UnknownInstruction(mnemonic);
        }
    }

    return machine_code;
}

/*
 * 判断机器码是否为 I 格式：
 */
bool isI_Format(MachineCode machine_code) {
    int op = machine_code >> 26;

    // 1. 算术/逻辑立即数指令: 0b001xxx (8 ~ 15)
    // 包括: addi, addiu, slti, sltiu, andi, ori, xori, lui
    if ((op >> 3) == 1) return true;

    // 2. 分支指令 (Branch): 0b0001xx (4 ~ 7) 和 0b000001 (1)
    // 包括: beq, bne, blez, bgtz, 以及 REGIMM 类 (bltz, bgez, bltzal, bgezal)
    if (op == 0b000001 || (op >> 2) == 1) return true;

    // 3. 访存指令 (Load/Store): 0b10xxxx (32 ~ 47)
    // 包括: lb, lh, lw, lbu, lhu, sb, sh, sw 等
    if ((op >> 4) == 0b10) return true;

    return false;
}

/*
 * 正则判断汇编指令是否 I 格式
 */
bool isI_Format(const std::string& assembly) {
    std::string mnemonic = GetMnemonic(assembly);
    std::cmatch m;
    std::regex_match(mnemonic.c_str(), m, I_format_regex);
    return (!m.empty() && m.prefix().str().empty() && m.suffix().str().empty());
}