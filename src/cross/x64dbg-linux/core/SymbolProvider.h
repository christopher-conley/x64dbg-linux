#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

namespace X64DbgLinux {

struct SymbolInfo {
    std::string name;
    uint64_t address;
    size_t size;
    int type;  // 0=function, 1=data, etc.
};

struct ModuleInfo {
    std::string name;
    std::string path;
    uint64_t base;
    size_t size;
    std::vector<SymbolInfo> symbols;
};

struct SourceLineInfo {
    std::string file;
    int line;
    std::string function;
};

class SymbolProvider {
public:
    // Load symbols from module
    bool loadModule(const std::string& path, uint64_t base);

    // Unload module symbols
    void unloadModule(uint64_t base);

    // Get symbol at address
    std::optional<SymbolInfo> getSymbolAt(uint64_t addr);

    // Get symbols in range
    std::vector<SymbolInfo> getSymbolsInRange(uint64_t start, uint64_t end);

    // Get source line info (requires DWARF)
    std::optional<SourceLineInfo> getSourceLine(uint64_t addr);

    // Get module info
    std::optional<ModuleInfo> getModuleInfo(uint64_t addr);

    // Get all loaded modules
    std::vector<ModuleInfo> getAllModules() const;

    // Resolve symbol name to address
    std::optional<uint64_t> resolveSymbol(const std::string& name, const std::string& module = "");

    // Clear all symbols
    void clearAll();

private:
    // ELF parsing
    bool parseElf(const std::string& path, uint64_t base, ModuleInfo& module);

    // DWARF parsing (optional)
    bool parseDwarf(const std::string& path, ModuleInfo& module);

    // Symbol table parsing
    bool parseSymtab(const std::string& path, ModuleInfo& module);

    // Dynamic symbol table parsing
    bool parseDynsym(const std::string& path, ModuleInfo& module);

    std::unordered_map<uint64_t, ModuleInfo> m_modules;
};

}
