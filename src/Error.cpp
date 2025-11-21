#include "Headers.h"

/*
 * 本文件为异常类的实现，全部为小型构造函数，
 * 在构造时向 std::runtime_error 传递具体错误消息。
 */

ExceptNumberOrSymbol::ExceptNumberOrSymbol(const std::string &msg)
    /*
     * msg：出错的字段名称
     * 例如：ExceptNumberOrSymbol("offset")
     * 会生成：
     *   "offset should be a number or a symbol."
     */
    : std::runtime_error(msg + " should be a number or a symbol."){};

ExceptNumber::ExceptNumber(const std::string &msg)
    : std::runtime_error(msg + " should be a number."){};

ExceptPositive::ExceptPositive(const std::string &msg)
    : std::runtime_error(msg + " should be a positive number."){};

ExceptRegister::ExceptRegister(const std::string &name)
    /*
     * 指令中给出的寄存器不是合法名称（如 “r33”、“ax”）
     */
    : std::runtime_error(name + " is not a register."){};

OperandError::OperandError(const std::string &mnemonic, const std::string &msg)
    /*
     * 用于描述指令操作数错误，如格式不对、参数类型不符等：
     * 例：
     *   OperandError("ADD", "expected register")
     * 生成：
     *   "expected register (ADD)."
     */
    : std::runtime_error(msg + " (" + mnemonic + ")."){};

UnkonwInstruction::UnkonwInstruction(const std::string &mnemonic)
    /*
     * 未知指令，如用户输入：
     *   foo r1, r2
     */
    : std::runtime_error("Unkonw instruction: " + mnemonic + "."){};

NumberOverflow::NumberOverflow(const std::string &name, const std::string &max,
                               const std::string &now)
    /*
     * 用于立即数溢出或地址超过范围
     *   name：字段名称，如 "offset"
     *   max：允许的最大值，如 "32767"
     *   now：当前值，如 "40000"
     */
    : std::runtime_error(name + " is too large. It should not larger than " +
                         max + ". Now it is " + now){};