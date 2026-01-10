#pragma once

/*
 * R 格式指令（Register Format）处理模块
 *
 * R 格式结构：
 *   31-26 | 25-21 | 20-16 | 15-11 | 10-6 | 5-0
 *      0  |  RS   |   RT  |   RD  | Shamt| Func
 *
 * 指令种类最多，包括：
 *   - 算术 ADD SUB AND OR SLT
 *   - 移位 SLL SRL SRA SLLV ...
 *   - 乘除 MULT DIV
 *   - 跳转 JR JALR
 *   - 系统 BREAK SYSCALL
 */

extern std::regex R_format_regex;

MachineCode R_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeIt machine_code_it,
                                Instruction* cur_instruction);

/*
 * R 格式判断
 */
bool isR_Format(MachineCode machine_code);
bool isR_Format(const std::string& assembly);