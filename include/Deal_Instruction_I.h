#pragma once

/*
 * I 格式指令（Immediate Format）处理模块
 * 包含：
 *   - I 格式指令的正则表达式
 *   - I 格式编码函数
 *   - I 格式指令判别函数
 *
 * I 格式结构（MIPS）：
 *   31-26 | 25-21 | 20-16 | 15-0
 *     OP  |   RS  |   RT  | Immediate
 */

extern std::regex I_format_regex;

/*
 * I_FormatInstruction
 *   输入：助记符 mnemonic、汇编文本 assembly
 *   输出：编码好的 32bit MachineCode
 *   功能：
 *     - 对不同 I 格式指令分类处理（算术/逻辑、分支、加载存储、COP0）
 *     - 管理未解析符号（保存到 unsolved_symbol_map）
 *     - 调用 SetOP/SetRS/SetRT/SetImmediate 写入字段
 *
 * machine_code_it：
 *     指向当前指令 machine_code 的迭代器，用于直接修改机器码
 */
MachineCode I_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeHandle machine_code_it);

/*
 * isI_Format：
 *   判断一条机器码或一条汇编指令是否属于 I 格式
 */
bool isI_Format(MachineCode machine_code);
bool isI_Format(const std::string& assembly);