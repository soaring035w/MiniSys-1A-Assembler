#pragma once

/*
 * 本文件定义了一系列汇编器用到的异常种类，用于描述具体的错误类型，
 * 例如：寄存器错误、数字溢出、操作数错误等。
 *
 * 每个异常继承自 std::runtime_error，以便在解析和生成机器码时抛出。
 */

class ExceptNumberOrSymbol : public std::runtime_error {
   public:
    // 期望一个“数字或符号”时抛出
    explicit ExceptNumberOrSymbol(const std::string &msg);
};

class ExceptNumber : public std::runtime_error {
   public:
    // 期望一个“纯数字”时抛出
    explicit ExceptNumber(const std::string &msg);
};

class ExceptPositive : public std::runtime_error {
   public:
    // 期望“正数”时抛出
    explicit ExceptPositive(const std::string &msg);
};

class ExceptRegister : public std::runtime_error {
   public:
    // 传入的字符串不是寄存器名时报错
    explicit ExceptRegister(const std::string &name);
};

class OperandError : public std::runtime_error {
   public:
    // 一般操作数错误，如格式不对、类型不对等
    explicit OperandError(const std::string &mnemonic,
                          const std::string &msg = "Invalid operation");
};

class TooManyOperand : public OperandError {
   public:
    // 操作数过多（如 add 只允许三个参数）
    explicit TooManyOperand(const std::string &mnemonic)
        : OperandError(mnemonic, "Too mamy operands"){};
};

class UnkonwInstruction : public std::runtime_error {
   public:
    // 未知指令
    explicit UnkonwInstruction(const std::string &mnemonic);
};

class NumberOverflow : public std::runtime_error {
   public:
    // 数字超出合法范围，如立即数超过 bit 宽度
    explicit NumberOverflow(const std::string &name, const std::string &max,
                            const std::string &now);
};