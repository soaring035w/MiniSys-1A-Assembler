#pragma once

/*
 * 工具模块
 *
 * 包含多个与解析汇编字符串相关的重要函数：
 *
 *  - toUppercase            将字符串转为大写
 *  - isNumber               判断是否为合法数字（支持 0x 十六进制）
 *  - isPositive             判断是否为非负整数（包含十六进制表示）
 *  - isDecimal              判断是否为十进制整数
 *  - toNumber               将字符串转换为 int（带符号）
 *  - toUNumber              将字符串转换为无符号整数
 *  - isSymbol               判断字符串是否为符号（标签）
 *  - isMemory               判断是否为 offset(base) 格式的内存操作
 * 这些函数在汇编指令解析和处理过程中确保输入的合法性和正确转换
 */

std::string toUppercase(std::string str);
bool isNumber(const std::string& str);
bool isPositive(const std::string& str);
bool isDecimal(const std::string& str);
int toNumber(const std::string& str, bool enable_hex = true);
unsigned toUNumber(const std::string& str, bool enable_hex = true);
bool isSymbol(const std::string& str);
bool isMemory(const std::string& str);