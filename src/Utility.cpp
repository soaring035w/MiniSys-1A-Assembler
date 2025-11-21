#include "Headers.h"

/*
 * toUppercase
 * 将字符串中所有 a~z 转为 A~Z。
 * 只处理 ASCII 范围，效率高。
 */
std::string toUppercase(std::string str) {
    for (auto& c : str) {
        if (c <= 'z' && c >= 'a') {
            c += 'A' - 'a';
        }
    }
    return str;
}

/*
 * isNumber
 *
 * 判断字符串是否为合法数字形式，包括：
 *   - "123"
 *   - "-123"
 *   - "0xFF"
 *   - "-0x2a"
 *
 * 结构：
 *   如果首字符是 '-'，则判断剩余部分 isPositive
 */
bool isNumber(const std::string& str) {
    if (str[0] == '-') {
        return isPositive(str.substr(1));
    } else {
        return isPositive(str);
    }
}

/*
 * isPositive
 *
 * 判断是否为合法“非负整数”，支持两种形式：
 *   - 十进制数字：    123
 *   - 十六进制数字：  0x1f2a
 *
 * 使用正则：
 *   ^(?:\d+|0x[0-9abcdef]+)$
 */
bool isPositive(const std::string& str) {
    static std::regex re(R"(^(?:\d+|0x[0-9abcdef]+)$)", std::regex::icase);
    std::cmatch m;
    std::regex_search(str.c_str(), m, re);
    return m[0].matched;
}

/*
 * isDecimal
 * 判断字符串是否为一个（可带 -）纯十进制数字。
 */
bool isDecimal(const std::string& str) {
    static std::regex re(R"(^\d+$)", std::regex::icase);
    std::cmatch m;

    if (str[0] == '-') {
        std::regex_search(str.substr(1).c_str(), m, re);
    } else {
        std::regex_search(str.c_str(), m, re);
    }

    return m[0].matched;
}

/*
 * toNumber
 *
 * 将字符串解析为 int 型。支持：
 *   - 十进制
 *   - 十六进制（0x）
 *
 * enable_hex = false → 禁止使用 0x 前缀
 *
 * 使用 std::stol / std::stoul 自动处理溢出与类型。
 */
int toNumber(const std::string& str, bool enable_hex) {
    if (!isNumber(str))
        throw std::runtime_error(str + " is not a number.");

    int ans;

    try {
        ans = std::stol(str, 0, enable_hex ? 0 : 10); // base=0 自动识别 0x
    } catch (const std::out_of_range& e) {
        try {
            ans = (int)std::stoul(str, 0, enable_hex ? 0 : 10);
        } catch (const std::out_of_range& e) {
            throw std::out_of_range("Number out of range.");
        }
    }
    return ans;
}

/*
 * toUNumber
 *
 * 将字符串转换为 unsigned，无符号值。
 * 同样支持十六进制数字。
 */
unsigned toUNumber(const std::string& str, bool enable_hex) {
    if (!isNumber(str))
        throw std::runtime_error(str + " is not a number.");

    unsigned ans;
    try {
        ans = (unsigned)std::stoul(str, 0, enable_hex ? 0 : 10);
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("Number out of range.");
    }

    return ans;
}

/*
 * isSymbol
 *
 * 判断字符串是否为“合法符号”，用于标签引用：
 *   - 由字母/数字/下划线/点/$ 组成
 *   - 不以数字或寄存器开头
 *   - 不是纯数字
 */
bool isSymbol(const std::string& str) {
    if (str.empty()) return false;

    std::regex re(R"(^[a-z0-9_.$]+$)", std::regex::icase);
    std::cmatch m;
    std::regex_search(str.c_str(), m, re);

    // 必须满足：
    // - 符号规则匹配
    // - 第一个字符不是数字
    // - 不是寄存器
    return !m.empty() &&
           !isPositive(str.substr(0, 1)) &&
           !isRegister(str);
}

/*
 * isMemory
 *
 * 判断是否为 offset(base) 格式的内存操作，如：
 *   "4($t0)"
 *   "-16($sp)"
 *   "var($s1)"
 *
 * 正则分组：
 *   m[1] → offset（数字或符号）
 *   m[2] → base   （寄存器）
 */
bool isMemory(const std::string& str) {
    std::regex re(R"(^\s*(\S+)\((\S+)\)\s*$)", std::regex::icase);
    std::cmatch m;
    std::regex_search(str.c_str(), m, re);

    if (!m.empty() &&
        (isNumber(m[1].str()) || isSymbol(m[1].str())) &&
        isRegister(m[2].str())) {
        return true;
    } else {
        return false;
    }
}