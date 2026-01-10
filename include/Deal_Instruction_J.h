#pragma once

/*
 * J 格式指令（Jump Format）处理模块
 *
 * J 格式结构：
 *   31-26 | 25-0
 *     OP  | Address
 *
 * 典型指令：
 *   J   addr
 *   JAL addr
 */

extern std::regex J_format_regex;

MachineCode J_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeIt machine_code_it);

/*
 * 判断指令是否是 J 格式
 */
bool isJ_Format(MachineCode machine_code);
bool isJ_Format(const std::string& assembly);