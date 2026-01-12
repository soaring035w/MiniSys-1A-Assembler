#pragma once

/*
 * 宏指令（Macro Instruction）处理模块
 * 本项目支持的宏指令包括：
 *   - MOV  : 多种寄存器/立即数/内存间复制模式
 *   - PUSH : 调整栈指针 + 保存寄存器
 *   - POP  : 取回栈上的寄存器值 + 调整栈指针
 *   - NOP  : 空操作（SLL $0,$0,0 模拟）
 */

extern std::regex Macro_format_regex;

/*
 * Macro_FormatInstruction：
 *
 * 参数：
 *   mnemonic：宏指令助记符（mov/push/pop/nop）
 *   assembly：完整汇编语句
 *   unsolved_symbol_map：未解决符号表（符号回填用）
 *   machine_code_it：当前 machine_code 的位置（展开可能改变它）
 *
 * 返回：
 *   展开后的第一条真实机器码
 *   - cur_instruction
 *   - cur_address
 *   - machine_code_it 起始位置
 */
MachineCode Macro_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeIt& machine_code_it,
                                unsigned int& cur_address,
                                Instruction* cur_instruction);

/*
 * 判断一条汇编语句是否是宏指令（MOV/PUSH/POP/NOP）
 */
bool isMacro_Format(const std::string& assembly);