#pragma once

/*
 *  doAssemble：汇编器主入口函数
 *
 * 参数：
 *  - input_file_path：输入的源汇编文件 (.s 或 .asm)
 *  - output_folder_path：输出文件路径，默认当前目录下
 *  - options：附加选项（如是否显示 details）
 *
 * 返回值：
 *  - 0：成功
 *  - 非 0：失败
 */
int doAssemble(const std::string &input_file_path,
               const std::string &output_folder_path = "./");