#include "Headers.h"

/*
 * NameToId：
 *   将别名寄存器（zero、t0、sp…）转换为对应编号。
 *   支持多种写法，如：
 *     $k0 / $i0 → 26
 *     $gp / $s9 → 28
 */
int NameToId(const std::string& str2) {

    std::string str3 = toUppercase(str2);

    if (str3 == "ZERO") return 0;
    else if (str3 == "AT") return 1;
    else if (str3 == "V0") return 2;
    else if (str3 == "V1") return 3;
    else if (str3 == "A0") return 4;
    else if (str3 == "A1") return 5;
    else if (str3 == "A2") return 6;
    else if (str3 == "A3") return 7;
    else if (str3 == "T0") return 8;
    else if (str3 == "T1") return 9;
    else if (str3 == "T2") return 10;
    else if (str3 == "T3") return 11;
    else if (str3 == "T4") return 12;
    else if (str3 == "T5") return 13;
    else if (str3 == "T6") return 14;
    else if (str3 == "T7") return 15;
    else if (str3 == "S0") return 16;
    else if (str3 == "S1") return 17;
    else if (str3 == "S2") return 18;
    else if (str3 == "S3") return 19;
    else if (str3 == "S4") return 20;
    else if (str3 == "S5") return 21;
    else if (str3 == "S6") return 22;
    else if (str3 == "S7") return 23;
    else if (str3 == "T8") return 24;
    else if (str3 == "T9") return 25;

    // 多名字寄存器
    else if (str3 == "K0" || str3 == "I0") return 26;
    else if (str3 == "K1" || str3 == "I1") return 27;

    else if (str3 == "GP" || str3 == "S9") return 28;

    else if (str3 == "SP") return 29;
    else if (str3 == "FP" || str3 == "S8") return 30;
    else if (str3 == "RA") return 31;

    else return -1;
}

/*
 * Register：
 *   解析寄存器（如 "$t1" 或 "$5"）
 *   返回寄存器编号（0~31），失败抛出异常
 */
int Register(const std::string& str) {

    // 去掉前缀 '$'
    std::string str2 = str.substr(1);

    try {
        // 数字形式（$0~$31）
        if (isPositive(str2) && isDecimal(str2) && toNumber(str2, false) < 32)
            return toNumber(str2, false);

        else {
            // 别名形式
            int id = NameToId(str2);
            if (id != -1)
                return id;
            else
                throw ExceptRegister(str);
        }
    } catch (const std::out_of_range& e) {
        throw ExceptRegister(str);
    }
}

/*
 * 判断是否是合法寄存器
 */
bool isRegister(const std::string& str) {

    std::string str2 = str.substr(1);

    try {
        return str[0] == '$' &&
               ((isPositive(str2) && isDecimal(str2) && (toNumber(str2) < 32)) ||
                NameToId(str2) != -1);

    } catch (const std::out_of_range& e) {
        return false;
    }
}