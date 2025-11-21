#include "Headers.h"

/*
 * J 格式（Jump Format）指令识别正则表达式
 *
 * 包含指令：
 *   J   label/imm
 *   JAL label/imm
 *
 * 注意：正则只匹配助记符，不匹配参数。
 */
std::regex J_format_regex("^(j|jal)", std::regex::icase);

/*
 * J_FormatInstruction
 *
 * 参数说明：
 *   mnemonic：助记符 "J" or "JAL"
 *   assembly：完整的汇编语句
 *   unsolved_symbol_map：用于存放需要第二遍填补的符号
 *   machine_code_it：指向本条指令 machine_code 数组中的位置
 *
 * J 指令格式：
 *   31-26 | 25-0
 *     OP  | address
 *
 * address 是 26 位，但实际跳转地址 = address * 4（因为 PC 对齐）
 */
MachineCode J_FormatInstruction(const std::string& mnemonic,
                                const std::string& assembly,
                                UnsolvedSymbolMap& unsolved_symbol_map,
                                MachineCodeHandle machine_code_it) {

    MachineCode& machine_code = *machine_code_it;

    // 首先判断是否确实为 J 格式
    if (isJ_Format(assembly)) {
        machine_code = 0;  // 重置机器码

        // 解析参数（最多三个）
        std::string op1, op2, op3;
        GetOperand(assembly, op1, op2, op3);

        /*
         * J / JAL 语法格式：
         *     J   target
         *     JAL target
         *
         * target 为数字 或 标签，且不能再有多余操作数
         */
        if ((isNumber(op1) || isSymbol(op1)) && op2.empty() && op3.empty()) {

            // 设置 OP 字段
            if (mnemonic == "J") {
                SetOP(machine_code, 0b000010);  // J
            } else {
                SetOP(machine_code, 0b000011);  // JAL
            }

            /*
             * op1 为跳转目的地址：
             *
             * 若是数字 → 直接写入（并给出警告）
             * 若是符号 → 写临时 0 占位，并加入未解决符号表
             */
            if (isNumber(op1)) {
                SetAddress(machine_code, toNumber(op1));

                std::cout<< "You are using an immediate value in jump instruction, ";
            } else {
                // 符号地址需第二遍回填
                SetAddress(machine_code, 0);
                unsolved_symbol_map[op1].push_back(
                    SymbolRef{machine_code_it, cur_instruction});
            }

        } else {
            /*
             * J 型最多只有 1 个参数，如果有2-3个参数 → 错误
             * 如果唯一参数不是数字或符号 → 错误
             */
            if (op2.empty() && op3.empty())
                throw ExceptNumberOrSymbol(op1);
            else
                throw TooManyOperand(mnemonic);
        }

    } else {
        // 助记符不属于 J 型指令
        throw UnkonwInstruction(mnemonic);
    }

    return machine_code;
}

/*
 * 判断机器码是否为 J 格式
 *
 * J (000010)
 * JAL (000011)
 */
bool isJ_Format(MachineCode machine_code) {
    int op = machine_code >> 26;
    return op == 0b000010 || op == 0b000011;
}

/*
 * 判断汇编语句是否为 J 格式
 */
bool isJ_Format(const std::string& assembly) {
    std::string mnemonic = GetMnemonic(assembly);
    std::cmatch m;
    std::regex_match(mnemonic.c_str(), m, J_format_regex);

    // 如果正则完整匹配（前缀后缀均为空）
    return (!m.empty() && m.prefix().str().empty() && m.suffix().str().empty());
}