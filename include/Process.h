#pragma once

/*
 * Process.h
 *
 * 本模块包含汇编器核心的“处理流程函数”，这些函数在 doAssemble 中被调用：
 *
 *  - GeneratedMachineCode：处理 .text 段，执行第一遍扫描生成机器码
 *  - GeneratedDataSegment：处理 .data 段，执行第一遍扫描生成原始数据
 *  - ProcessInstruction：把某一行指令解析成机器码
 *  - ProcessData：处理 BYTE / HALF / WORD 等伪指令
 *  - ProcessLabel：识别标签并加入 symbol_map
 *  - KillComment：去掉注释
 *  - SolveSymbol：第二遍扫描，处理所有前向引用符号
 *
 * 其中两个外部变量：
 *  - cur_instruction：当前正在处理的指令（用于错误提示）
 *  - cur_address：当前地址累加器（每生成一条机器码 +4）
 */

int GeneratedMachineCode(InstructionList& instruction_list,
                         UnsolvedSymbolMap& unsolved_symbol_map,
                         SymbolMap& symbol_map);

int GeneratedDataSegment(DataList& data_list, SymbolMap& symbol_map);

void ProcessInstruction(const std::string& assembly, Instruction& instruction,
                     UnsolvedSymbolMap& unsolved_symbol_map);

void ProcessData(const std::string& assembly, Data& data);

/*
 * ProcessLabel：
 *   从一行汇编语句中提取标签（例如 "Loop: addi $t1,$t1,1"）
 *   - 将标签加入 symbol_map[label] = address
 *   - 返回去掉标签后的剩余语句 "addi $t1,$t1,1"
 */
std::string ProcessLabel(unsigned int address, const std::string& assembly,
                         SymbolMap& symbol_map);

/*
 * KillComment：
 *   去掉 # 后面的内容
 */
std::string KillComment(const std::string& assembly);

/*
 * SolveSymbol：
 *   第二遍扫描，解决所有“引用符号的位置”
 *   - I 格式立即数
 *   - J 格式跳转地址
 *   - R 格式 shamt（极少见，一般用于测试）
 */
int SolveSymbol(UnsolvedSymbolMap& unsolved_symbol_map,
                const SymbolMap& symbol_map);

/*
 * 当前处理的指令（仅用于错误输出行号）
 * 当前地址（字节为单位）
 */
extern Instruction* cur_instruction;
extern unsigned cur_address;