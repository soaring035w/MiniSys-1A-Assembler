#include "Headers.h"

/*
 * NewMachineCode：
 *    在当前 Instruction 的 machine_code 数组末尾增加一个新的 32 位机器码（初始为 0）
 *    返回指向这个新 machine_code 的迭代器。
 *
 * 多数编码函数首先调用 NewMachineCode，然后再用 SetOP/SetRS 等函数填充字段。
 */
MachineCodeHandle NewMachineCode(Instruction& i) {
    i.machine_code.push_back(0);
    return i.machine_code.begin() + (i.machine_code.size() - 1);
}

/*
 * 以下函数操作 32bit MIPS 机器码，利用掩码(mask) 来清除旧字段，再写入新字段。
 * 
 * 各字段的 bit 分布：
 *   OP     (31~26)
 *   RS     (25~21)
 *   RT     (20~16)
 *   RD     (15~11)
 *   Shamt  (10~6)
 *   Func   (5~0)
 */

void SetOP(MachineCode& machine_code, unsigned OP) {
    if (OP >= 64) {  // 6 bit 上限
        throw NumberOverflow("OP", "63", std::to_string(OP));
    }
    // 清除 bit 31-26，然后写入 OP
    machine_code &= 0b00000011111111111111111111111111;
    machine_code |= OP << 26;
}

void SetRS(MachineCode& machine_code, unsigned RS) {
    if (RS >= 32) {  // 寄存器编号 5 bit
        throw NumberOverflow("RS", "31", std::to_string(RS));
    }
    machine_code &= 0b11111100000111111111111111111111;
    machine_code |= RS << 21;
}

void SetRT(MachineCode& machine_code, unsigned RT) {
    if (RT >= 32) {
        throw NumberOverflow("RT", "31", std::to_string(RT));
    }
    machine_code &= 0b11111111111000001111111111111111;
    machine_code |= RT << 16;
}

void SetRD(MachineCode& machine_code, unsigned RD) {
    if (RD >= 32) {
        throw NumberOverflow("RD", "31", std::to_string(RD));
    }
    machine_code &= 0b11111111111111110000011111111111;
    machine_code |= RD << 11;
}

void SetShamt(MachineCode& machine_code, unsigned shamt) {
    if (shamt >= 32) {
        throw NumberOverflow("Shamt", "31", std::to_string(shamt));
    }
    machine_code &= 0b11111111111111111111100000111111;
    machine_code |= shamt << 6;
}

void SetFunc(MachineCode& machine_code, unsigned func) {
    if (func >= 64) {  // 6 bit function code
        throw NumberOverflow("Function code", "31", std::to_string(func));
    }
    machine_code &= 0b11111111111111111111111111000000;
    machine_code |= func << 0;
}

/*
 * SetImmediate：
 *    写 I-format 的立即数字段 (bit 15~0)
 *
 * immediate 最终只保留低 16 bit
 * 注意：立即数未做符号扩展，这部分行为由调用者保证。
 */
void SetImmediate(MachineCode& machine_code, int immediate) {
    // int op = machine_code >> 26;
    if (immediate >= 65536) {    // 16 bit 极限
        throw NumberOverflow("Immediate", "65535", std::to_string(immediate));
    }
    machine_code &= 0xffff0000;       // 清空低 16 bit
    machine_code |= (immediate << 0) & 0xffff;
}

/*
 * SetAddress：
 *    写 J-format 的地址字段（bit 25~0，共 26 bit）
 */
void SetAddress(MachineCode& machine_code, unsigned address) {
    if (address >= 67108864) { // 2^26 = 67,108,864
        throw NumberOverflow("Address", "67108863", std::to_string(address));
    }
    machine_code &= 0b11111100000000000000000000000000;
    machine_code |= address << 0;
}

/*
 * GetMnemonic：
 *    使用正则取出汇编行的第一个 token，即助记符（指令名）
 */
std::string GetMnemonic(const std::string& assembly) {
    static std::regex re("^\\s*(\\S+)");
    std::smatch match;
    std::regex_search(assembly, match, re);
    return match[1].matched ? match[1].str() : "";
}

/*
 * GetOperand：
 *    利用三个不同正则表达式匹配 3/2/1 个操作数
 *
 * 示例：
 *   "add $t1, $t2, $t3" -> op1="$t1", op2="$t2", op3="$t3"
 *   "move $t1, $t2"     -> op1="$t1", op2="$t2", op3=""
 *   "jr $ra"            -> op1="$ra", op2="",   op3=""
 */
void GetOperand(const std::string& assembly, std::string& op1, std::string& op2,
                std::string& op3) {
    static std::regex re_3("\\s*\\S+\\s+(\\S+)\\s*,\\s*(\\S+)\\s*,\\s*(\\S+)",
                           std::regex::icase),  // 3 操作数
        re_2("\\s*\\S+\\s+(\\S+)\\s*,\\s*(\\S+)",
             std::regex::icase),                // 2 操作数
        re_1("\\s*\\S+\\s+(\\S+)", std::regex::icase);  // 1 操作数

    std::smatch match;

    // ----- 三操作数 -----
    std::regex_match(assembly, match, re_3);
    if (!match.empty()) {
        op1 = match[1].str();
        op2 = match[2].str();
        op3 = match[3].str();
        return;
    }

    // ----- 二操作数 -----
    op3 = "";
    std::regex_match(assembly, match, re_2);
    if (!match.empty()) {
        op1 = match[1].str();
        op2 = match[2].str();
        return;
    }

    // ----- 一操作数 -----
    op2 = "";
    std::regex_match(assembly, match, re_1);
    if (!match.empty()) {
        op1 = match[1].str();
        return;
    }

    // ----- 没有参数 -----
    op1 = "";
}