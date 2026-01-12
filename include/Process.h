#pragma once

/*
 * AssemblerCore 类
 * 封装汇编器的核心处理逻辑
 */
class AssemblerCore {
public:
    AssemblerCore() : current_address(0), has_error(false) {}

    // 第一遍扫描：处理 .text 段
    bool ProcessTextSegment(InstructionList& instruction_list,
                            UnsolvedSymbolMap& unsolved_symbol_map,
                            SymbolMap& symbol_map);

    // 第一遍扫描：处理 .data 段
    bool ProcessDataSegment(DataList& data_list,
                            SymbolMap& symbol_map);

    // 第二遍扫描：符号解析与回填
    bool ResolveSymbols(UnsolvedSymbolMap& unsolved_symbol_map,
                        const SymbolMap& symbol_map);

private:
    // 内部状态
    unsigned int current_address; // 当前地址指针
    bool has_error; // 是否发生错误
    const Instruction* current_instruction_ptr = nullptr; // 当前处理的指令指针

    // 辅助函数
    // 提取标签并去除注释
    std::string ExtractLabelAndStripComment(unsigned int address,
                                            const std::string& assembly,
                                            SymbolMap& symbol_map);
    
    // 分发指令和数据处理
    void DispatchInstruction(const std::string& assembly, 
                             Instruction& instruction,
                             UnsolvedSymbolMap& unsolved_symbol_map);
    
    void DispatchData(const std::string& assembly, Data& data);

    // 工具函数
    // 检查是否为分支指令
    bool IsBranchOpcode(int op) const;
    // 记录错误信息
    void LogError(const std::string& msg, const std::string& context = "");
};