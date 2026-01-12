#include "Headers.h"

/*
 * 支持的宏指令正则表达式：
 *   mov / push / pop / nop
 */
std::regex Macro_format_regex("^(mov|push|pop|nop)", std::regex::icase);

/*
 * Macro_FormatInstruction
 *
 * 功能：
 *   将一条宏指令展开为若干条真实 I/R 格式指令。
 *
 * 实现原理：
 *   - MOV 分情况展开：
 *       mov r1, r2       →  or r1, $0, r2
 *       mov r1, imm      →  ori r1, $0, imm  或者用 lui/ori 组合处理大立即数
 *       mov r1, (rs)     →  lw
 *       mov (rs), r1     →  sw
 *
 *   - PUSH reg：
 *       addi $sp,$sp,-4
 *       sw   reg,0($sp)
 *
 *   - POP reg：
 *       lw   reg,0($sp)
 *       addi $sp,$sp,4
 *
 *   - NOP：
 *       sll  $0,$0,0
 *
 * 其中某些展开操作会修改 machine_code_it，因为会新增 machine_code。
 */
MachineCode Macro_FormatInstruction(const std::string& mnemonic,
                                    const std::string& assembly,
                                    UnsolvedSymbolMap& unsolved_symbol_map,
                                    MachineCodeIt& machine_code_it,
                                    unsigned int& cur_address,
                                    Instruction* cur_instruction) {

    std::string op1, op2, op3;
    GetOperand(assembly, op1, op2, op3);

    
    // 一、 MOV 宏指令（三种情况：寄存器间、寄存器与内存、寄存器与立即数）
    
    if (mnemonic == "MOV") {

        // MOV 不应有三操作数
        if (!op3.empty()) {
        } else {

            // mov r1, r2  →  or r1, $0, r2
            if (isRegister(op1) && isRegister(op2)) {

                R_FormatInstruction(
                    "OR",
                    "OR " + op1 + ", $0, " + op2,
                    unsolved_symbol_map,
                    machine_code_it,
                    cur_instruction);
            }

            // mov r1, offset(rs) → lw r1, offset(rs)
            else if (isRegister(op1) && isMemory(op2)) {

                I_FormatInstruction(
                    "LW",
                    "LW " + op1 + ", " + op2,
                    unsolved_symbol_map,
                    machine_code_it,
                    cur_instruction);
            }

            // mov offset(rs), r2 → sw r2, offset(rs)
            else if (isMemory(op1) && isRegister(op2)) {

                I_FormatInstruction(
                    "SW",
                    "SW " + op2 + ", " + op1,
                    unsolved_symbol_map,
                    machine_code_it,
                    cur_instruction);
            }

            // mov r1, imm(symbol)
            else if (isRegister(op1) && (isNumber(op2) || isSymbol(op2))) {

                // 判断立即数是否超过 16 位范围，如果超过需要用两条指令拆分
                bool is_large_num = (!isSymbol(op2)) && (toUNumber(op2) > 0xffff);

                if (is_large_num) {
                    /*
                     * 对大立即数（超过 16 bit）：
                     *   mov r, imm32
                     * 展开为：
                     *   lui r, imm[31:16]
                     *   ori r, r, imm[15:0]
                     */

                    unsigned number = toUNumber(op2);

                    // 新增一条 machine_code，用于后续 ORI
                    MachineCodeIt new_handel = NewMachineCode(*cur_instruction);

                    // machine_code_it 放到当前指令对应的第一条 machine_code
                    machine_code_it = cur_instruction->machine_code.begin();

                    // 高 16 bit
                    I_FormatInstruction(
                        "LUI",
                        "LUI " + op1 + ", " + std::to_string(number >> 16),
                        unsolved_symbol_map,
                        machine_code_it,
                        cur_instruction
                    );

                    // 低 16 bit
                    I_FormatInstruction(
                        "ORI",
                        "ORI " + op1 + ", " + op1 + ", " +
                            std::to_string(number % 0x10000),
                        unsolved_symbol_map,
                        new_handel,
                        cur_instruction
                    );

                    // 指令地址递增（因为新增了指令）
                    cur_address += 4;
                }

                // 立即数未超 16 bit，使用 ORI
                else {
                    I_FormatInstruction(
                        "ORI",
                        "ORI " + op1 + ", $0, " + op2,
                        unsolved_symbol_map,
                        machine_code_it,
                        cur_instruction
                    );
                }

            } else {
                goto err;
            }
        }
    }

    
    // 二、 PUSH reg → addi $sp,$sp,-4    sw reg,0($sp)
    
    else if (mnemonic == "PUSH") {
        if (!op1.empty() && op2.empty() && op3.empty()) {

            // 展开为两条指令，因此新增 machine_code
            MachineCodeIt new_handel = NewMachineCode(*cur_instruction);

            // 第一条 ADDI 写入第一段 machine_code
            machine_code_it = cur_instruction->machine_code.begin();

            I_FormatInstruction("ADDI",
                                "ADDI $sp, $sp, -4",
                                unsolved_symbol_map,
                                machine_code_it,
                                cur_instruction);

            // 第二条 SW 使用新 handle
            I_FormatInstruction("SW",
                                "SW " + op1 + ", 0($sp)",
                                unsolved_symbol_map,
                                new_handel,
                                cur_instruction);

            cur_address += 4; // 新增一条指令
        } else {
            throw OperandError(mnemonic);
        }
    }

    
    // POP reg → lw reg,0($sp)   addi $sp,$sp,4
    
    else if (mnemonic == "POP") {
        if (!op1.empty() && op2.empty() && op3.empty()) {

            MachineCodeIt new_handel = NewMachineCode(*cur_instruction);
            machine_code_it = cur_instruction->machine_code.begin();

            I_FormatInstruction("LW",
                                "LW " + op1 + ", 0($sp)",
                                unsolved_symbol_map,
                                machine_code_it,
                                cur_instruction);

            I_FormatInstruction("ADDI",
                                "ADDI $sp, $sp, 4",
                                unsolved_symbol_map,
                                new_handel,
                                cur_instruction);

            cur_address += 4;
        } else {
            throw OperandError(mnemonic);
        }
    }

    
    // NOP → SLL $0,$0,0
    
    else if (mnemonic == "NOP") {
        R_FormatInstruction("SLL",
                            "SLL $0, $0, 0",
                            unsolved_symbol_map,
                            machine_code_it,
                            cur_instruction);
    }
    // 不支持的宏指令
    else {
    err:
        if (isMacro_Format(assembly))
            throw OperandError(mnemonic);
        else
            throw UnknownInstruction(mnemonic);
    }

    // 返回此宏指令展开后的第一条机器码
    return cur_instruction->machine_code.front();
}

/*
 * 判定是否是宏指令
 */
bool isMacro_Format(const std::string& assembly) {
    std::string mnemonic = GetMnemonic(assembly);
    std::cmatch m;
    std::regex_match(mnemonic.c_str(), m, Macro_format_regex);

    // 必须完整匹配助记符
    return (!m.empty() && m.prefix().str().empty() && m.suffix().str().empty());
}