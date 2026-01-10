#pragma once

/*
 * 输出模块：
 *
 * OutputInstruction(): 将指令段输出为 .coe 文件
 *
 * OutputDataSegment(): 将数据段输出为 .coe 文件
 * ShowDetails()：
 *   - 遍历所有指令 InstructionList
 *   - 显示每条指令的：
 *       offset（地址偏移）
 *       machine code（十六进制）
 *       machine code（32位二进制）
 *       assembly（原汇编文本）
 *
 *   - 遍历所有数据 DataList
 *   - 显示每个原始字节 raw_data：
 *       offset（地址偏移）
 *       字节值（两位十六进制）
 *       字节值（二进制）
 *       assembly（原汇编文本）
 */
const int TOTAL_WORDS = 16384; // 总共输出的字数（每字 4 字节，最多64KB）

void OutputInstruction(std::ostream& out,
                       const InstructionList &instruction_list);

void OutputDataSegment(std::ostream& out,
                       const DataList& instruction_list);

void OutputDetails(InstructionList instruction_list, DataList data_list,
                 std::ostream& out = std::cerr);