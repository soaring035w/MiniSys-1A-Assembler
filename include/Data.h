#pragma once

using BYTE = std::uint8_t;

/*
 * Data 结构用于描述 `.data` 段的一行数据。
 * 它记录了：
 * - assembly：该行的原始汇编文本
 * - file：所在文件名
 * - line：在源文件中的行号
 * - address：该行对应的数据在最终数据段中的起始地址（编译后确定）
 * - done：是否已经完成生成 raw_data（用于两遍扫描）
 * - raw_data：最终生成的数据字节序列（如 .word/.byte 的结果）
 *
 * DataList：是由多条 Data 记录组成的数组，表示整个数据段。
 */
struct Data {
    std::string assembly, file;  // 原始汇编语句、源文件路径
    int line;                    // 行号
    int address;                 // 数据在内存中的位置（第二遍确定）
    bool done = false;           // 是否已解析完成
    std::vector<BYTE> raw_data;  // 最终生成的二进制数据
};

using DataList = std::vector<Data>;