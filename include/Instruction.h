#pragma once

using MachineCode = std::uint32_t; 
// MachineCode 表示 32 位机器码（MIPS32 指令的最高位到最低位）

using MachineCodeHandle = std::vector<MachineCode>::iterator;
// MachineCodeHandle 是指向 machine_code 数组中某个具体机器码的迭代器。
// 这个句柄用于符号回填（修正跳转/立即数）。

/*
 * Instruction 结构体表示一行 .text 段中的指令。
 * 
 * 字段含义：
 * assembly：该行原始汇编文本
 * file：所在源文件名称
 * line：所在行号
 * address：该指令在最终程序中的地址（字节为单位）
 * done：是否已经生成 machine_code
 * machine_code：本指令最终生成的一条或多条机器码（宏指令可能展开成多条）
 */
struct Instruction {
    std::string assembly, file;
    unsigned line;
    unsigned address;
    bool done = false;
    std::vector<MachineCode> machine_code;
};

/*
 * SymbolRef 用于记录指令中“引用符号”的位置
 *
 * machine_code_handle：指向 machine_code 中需要回填的位置
 * instruction：该引用符号属于哪条 Instruction
 */
struct SymbolRef {
    MachineCodeHandle machine_code_handle;
    Instruction* instruction;
};

using InstructionList = std::vector<Instruction>;

/*
 * UnsolvedSymbolMap（未解决符号表）：
 *      key   = 符号名（如 Label）
 *      value = 所有引用该符号的 SymbolRef 列表
 *
 * 用于解决前向引用（第二遍修正）
 */
using UnsolvedSymbolMap =
    std::unordered_map<std::string, std::vector<SymbolRef>>;

/*
 * SymbolMap（已解决符号表）：
 *      key   = 符号名
 *      value = 符号对应的地址
 */
using SymbolMap = std::unordered_map<std::string, unsigned int>;

/*
 * NewMachineCode：
 *      在 Instruction.machine_code 末尾插入一个新的机器码（初始值 0）
 *      返回指向它的迭代器，用于后续写入 OP, RS, RT 等字段。
 */
MachineCodeHandle NewMachineCode(Instruction& i);

// 不同类型指令的处理模块
#include "Deal_Instruction_I.h"
#include "Deal_Instruction_J.h"
#include "Deal_Macro.h"
#include "Deal_Instruction_R.h"

/*
 * 以下函数用于向一个 32-bit machine_code 中写入对应字段。
 * 每个函数都负责：
 *    - 检查数值范围是否合法
 *    - 清空 machine_code 对应 bit 位
 *    - 写入新的值（使用左移和按位或）
 */
void SetOP(MachineCode& machine_code, unsigned OP);
void SetRS(MachineCode& machine_code, unsigned RS);
void SetRT(MachineCode& machine_code, unsigned RT);
void SetRD(MachineCode& machine_code, unsigned RD);
void SetShamt(MachineCode& machine_code, unsigned shamt);
void SetFunc(MachineCode& machine_code, unsigned func);
void SetImmediate(MachineCode& machine_code, int immediate);
void SetAddress(MachineCode& machine_code, unsigned address);

/*
 * 从汇编语句中提取助记符（mnemonic）
 * 例如： "  add $t1, $t2, $t3"
 * 返回： "add"
 */
std::string GetMnemonic(const std::string& assembly);

/*
 * 从汇编语句中解析最多 3 个操作数。
 *
 * 支持格式：
 *   op   a, b, c
 *   op   a, b
 *   op   a
 *
 * 若找不到某个操作数，则对应的 opX = ""。
 */
void GetOperand(const std::string& assembly, std::string& op1, std::string& op2,
                std::string& op3);