#pragma once

/*
 * Register 模块：
 *
 * 功能：
 *   - 识别寄存器名（$t0, $a1, $sp, $ra, $31…）
 *   - 将寄存器名转换为编号（0~31）
 *
 * 支持两种写法：
 *   - 数字型寄存器：$0 ~ $31
 *   - 别名寄存器：$t0, $a0, $s1, $sp, $ra, $gp 等
 */

int Register(const std::string& str);

/*
 * 判断一个字符串是否为寄存器
 */
bool isRegister(const std::string& str);