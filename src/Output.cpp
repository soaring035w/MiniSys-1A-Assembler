#include "Headers.h"

/*
 * 输出 COE 文件头
 */
static void OutputHeader(std::ostream& out) {
    out << R"(memory_initialization_radix = 16;
memory_initialization_vector =
)";
}

/*
 * OutputInstruction：
 *   输出 instruction_list 中所有 machine_code
 *   每个 machine_code 按 8 位十六进制输出
 */
void OutputInstruction(std::ostream& out,
                       const InstructionList& instruction_list) {
    OutputHeader(out);

    std::vector<uint32_t> mem(TOTAL_WORDS, 0);

    // ---- 把指令写入正确的地址 ----
    for (const auto& ins : instruction_list) {
        int word_addr = ins.address / 4;   // 字节地址 → 字地址
        for (size_t k = 0; k < ins.machine_code.size(); ++k) {
            if (word_addr + k < TOTAL_WORDS)
                mem[word_addr + k] = ins.machine_code[k];
        }
    }

    // ---- 输出 COE ----
    for (int i = 0; i < TOTAL_WORDS; ++i) {
        out << std::setw(8)
            << std::setfill('0')
            << std::hex << mem[i]
            << (i == TOTAL_WORDS - 1 ? ';' : ',') << "\n";
    }
}

/*
 * OutputDataSegment：
 *   4 字节组合为一个 32bit word 输出。
 *   raw_data 是 byte 存储，因此按每 4 byte 打包。
 */
void OutputDataSegment(std::ostream& out,
                       const DataList& data_list) {
    OutputHeader(out);

    std::vector<uint32_t> mem(TOTAL_WORDS, 0);

    // --- 写入 Data 段 ---
    for (const auto& d : data_list) {
        int word_addr = d.address / 4;      // 字节地址 → 字地址
        uint8_t buffer[4] = {0}; // 临时缓冲区，Data中的二进制数据是按byte存储的
        int bi = 0; // 缓冲区索引

        for (size_t i = 0; i < d.raw_data.size(); ++i) {
            buffer[bi++] = d.raw_data[i];

            if (bi == 4) {
                uint32_t word = *(uint32_t*)buffer;  // little-endian
                if (word_addr < TOTAL_WORDS)
                    mem[word_addr] = word;

                word_addr++;
                bi = 0;
                memset(buffer, 0, 4); // 清空缓冲区
            }
        }

        // ----- 若不足 4 字节，也要写一个 word -----
        if (bi != 0 && word_addr < TOTAL_WORDS) {
            uint32_t word = *(uint32_t*)buffer;
            mem[word_addr] = word;
        }
    }

    // ---- 输出 COE ----
    for (int i = 0; i < TOTAL_WORDS; ++i) {
        out << std::setw(8)
            << std::setfill('0')
            << std::hex << mem[i]
            << (i == TOTAL_WORDS - 1 ? ';' : ',') << "\n";
    }
}

void OutputDetails(InstructionList instruction_list, DataList data_list,
                 std::ostream& out) {

    // ==============================
    // Code Segment 输出
    // ==============================
    out << "Code Segment\n          Machine code\n"
        << "Offset    hex       bin                               \tassembly\n";

    for (const Instruction& instruction : instruction_list) {

        auto offset = instruction.address; // 起始地址（每条指令固定 4 字节）

        for (const auto machine_code : instruction.machine_code) {

            // Offset：8位十六进制（统一宽度）
            out << std::hex << std::setw(8) << std::setfill('0') << offset << "  ";

            // Machine code：8位十六进制
            out << std::setw(8) << std::setfill('0') << machine_code << "  ";

            // Machine code：32位二进制
            out << std::bitset<32>(machine_code) << "\t";

            // assembly：原始文本
            out << instruction.assembly << '\n';

            offset += 4;
        }
    }

    // ==============================
    // Data Segment 输出
    // ==============================
    out << "\nData Segment\n          Raw data\n"
        << "Offset    hex bin     \tassembly\n";

    for (const Data& data : data_list) {

        auto offset = data.address;

        for (const auto raw_data : data.raw_data) {

            // Offset：8位十六进制
            out << std::hex << std::setw(8) << std::setfill('0') << offset << "  ";

            // Raw data：2位十六进制（单个字节）
            out << std::setw(2) << std::setfill('0') << (unsigned)raw_data << "  ";

            // Raw data：8位二进制
            out << std::bitset<8>(raw_data) << "\t";

            // assembly：原始数据行
            out << data.assembly << '\n';

            offset += 1;
        }
    }
}