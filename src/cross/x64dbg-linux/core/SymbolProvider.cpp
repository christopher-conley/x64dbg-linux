#include "SymbolProvider.h"
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <algorithm>

namespace X64DbgLinux {

bool SymbolProvider::loadModule(const std::string& path, uint64_t base) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ModuleInfo module;
    module.path = path;
    module.base = base;

    // Extract module name from path
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        module.name = path.substr(lastSlash + 1);
    } else {
        module.name = path;
    }

    if (!parseElf(path, base, module)) {
        return false;
    }

    m_modules[base] = module;
    return true;
}

void SymbolProvider::unloadModule(uint64_t base) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_modules.erase(base);
}

std::optional<SymbolInfo> SymbolProvider::getSymbolAt(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Find which module contains this address
    for (const auto& [base, module] : m_modules) {
        if (addr >= base && addr < base + module.size) {
            uint64_t offset = addr - base;
            for (const auto& sym : module.symbols) {
                if (offset >= sym.address && offset < sym.address + sym.size) {
                    SymbolInfo result = sym;
                    result.address = base + sym.address;
                    return result;
                }
            }
        }
    }
    return std::nullopt;
}

std::vector<SymbolInfo> SymbolProvider::getSymbolsInRange(uint64_t start, uint64_t end) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<SymbolInfo> result;

    for (const auto& [base, module] : m_modules) {
        for (const auto& sym : module.symbols) {
            uint64_t absAddr = base + sym.address;
            if (absAddr >= start && absAddr < end) {
                SymbolInfo s = sym;
                s.address = absAddr;
                result.push_back(s);
            }
        }
    }

    return result;
}

std::optional<SourceLineInfo> SymbolProvider::getSourceLine(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // DWARF parsing would go here
    // For now, return empty
    (void)addr;
    return std::nullopt;
}

std::optional<ModuleInfo> SymbolProvider::getModuleInfo(uint64_t addr) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [base, module] : m_modules) {
        if (addr >= base && addr < base + module.size) {
            return module;
        }
    }
    return std::nullopt;
}

std::vector<ModuleInfo> SymbolProvider::getAllModules() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<ModuleInfo> result;
    for (const auto& [base, module] : m_modules) {
        result.push_back(module);
    }
    return result;
}

std::optional<uint64_t> SymbolProvider::resolveSymbol(const std::string& name, const std::string& module) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [base, mod] : m_modules) {
        if (!module.empty() && mod.name != module) {
            continue;
        }

        for (const auto& sym : mod.symbols) {
            if (sym.name == name) {
                return base + sym.address;
            }
        }
    }
    return std::nullopt;
}

void SymbolProvider::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_modules.clear();
}

bool SymbolProvider::parseElf(const std::string& path, uint64_t base, ModuleInfo& module) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return false;
    }

    void* mapped = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return false;
    }

    // Check ELF magic
    unsigned char* e_ident = static_cast<unsigned char*>(mapped);
    if (e_ident[EI_MAG0] != ELFMAG0 || e_ident[EI_MAG1] != ELFMAG1 ||
        e_ident[EI_MAG2] != ELFMAG2 || e_ident[EI_MAG3] != ELFMAG3) {
        munmap(mapped, st.st_size);
        close(fd);
        return false;
    }

    // Check if 64-bit
    if (e_ident[EI_CLASS] != ELFCLASS64) {
        munmap(mapped, st.st_size);
        close(fd);
        return false;
    }

    Elf64_Ehdr* ehdr = static_cast<Elf64_Ehdr*>(mapped);
    module.size = st.st_size;

    // Parse program headers to find load address
    Elf64_Phdr* phdr = reinterpret_cast<Elf64_Phdr*>(static_cast<char*>(mapped) + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD && phdr[i].p_flags & PF_X) {
            // Executable segment
            break;
        }
    }

    // Parse section headers for symbols
    if (ehdr->e_shoff != 0) {
        Elf64_Shdr* shdr = reinterpret_cast<Elf64_Shdr*>(static_cast<char*>(mapped) + ehdr->e_shoff);

        // Get section string table
        Elf64_Shdr* shstrtab = &shdr[ehdr->e_shstrndx];
        char* strtab = static_cast<char*>(mapped) + shstrtab->sh_offset;

        // Find symbol table and string table
        Elf64_Shdr* symtab_hdr = nullptr;
        Elf64_Shdr* strtab_hdr = nullptr;
        Elf64_Shdr* dynsym_hdr = nullptr;
        Elf64_Shdr* dynstr_hdr = nullptr;

        for (int i = 0; i < ehdr->e_shnum; i++) {
            const char* name = strtab + shdr[i].sh_name;
            if (strcmp(name, ".symtab") == 0) {
                symtab_hdr = &shdr[i];
            } else if (strcmp(name, ".strtab") == 0) {
                strtab_hdr = &shdr[i];
            } else if (strcmp(name, ".dynsym") == 0) {
                dynsym_hdr = &shdr[i];
            } else if (strcmp(name, ".dynstr") == 0) {
                dynstr_hdr = &shdr[i];
            }
        }

        // Parse symbol table
        if (symtab_hdr && strtab_hdr) {
            Elf64_Sym* symbols = reinterpret_cast<Elf64_Sym*>(static_cast<char*>(mapped) + symtab_hdr->sh_offset);
            char* strings = static_cast<char*>(mapped) + strtab_hdr->sh_offset;
            size_t num_symbols = symtab_hdr->sh_size / sizeof(Elf64_Sym);

            for (size_t i = 0; i < num_symbols; i++) {
                if (symbols[i].st_name == 0) continue;

                const char* name = strings + symbols[i].st_name;
                if (name[0] == '\0') continue;

                SymbolInfo sym;
                sym.name = name;
                sym.address = symbols[i].st_value;
                sym.size = symbols[i].st_size;

                // Determine type
                unsigned char type = ELF64_ST_TYPE(symbols[i].st_info);
                if (type == STT_FUNC) {
                    sym.type = 0; // function
                } else if (type == STT_OBJECT) {
                    sym.type = 1; // data
                } else {
                    continue; // Skip other types
                }

                module.symbols.push_back(sym);
            }
        }

        // Parse dynamic symbol table
        if (dynsym_hdr && dynstr_hdr) {
            Elf64_Sym* symbols = reinterpret_cast<Elf64_Sym*>(static_cast<char*>(mapped) + dynsym_hdr->sh_offset);
            char* strings = static_cast<char*>(mapped) + dynstr_hdr->sh_offset;
            size_t num_symbols = dynsym_hdr->sh_size / sizeof(Elf64_Sym);

            for (size_t i = 0; i < num_symbols; i++) {
                if (symbols[i].st_name == 0) continue;

                const char* name = strings + symbols[i].st_name;
                if (name[0] == '\0') continue;

                // Check if already in symbols
                bool found = false;
                for (const auto& sym : module.symbols) {
                    if (sym.name == name) {
                        found = true;
                        break;
                    }
                }
                if (found) continue;

                SymbolInfo sym;
                sym.name = name;
                sym.address = symbols[i].st_value;
                sym.size = symbols[i].st_size;

                unsigned char type = ELF64_ST_TYPE(symbols[i].st_info);
                if (type == STT_FUNC) {
                    sym.type = 0;
                } else if (type == STT_OBJECT) {
                    sym.type = 1;
                } else {
                    continue;
                }

                module.symbols.push_back(sym);
            }
        }
    }

    munmap(mapped, st.st_size);
    close(fd);

    // Sort symbols by address
    std::sort(module.symbols.begin(), module.symbols.end(),
        [](const SymbolInfo& a, const SymbolInfo& b) {
            return a.address < b.address;
        });

    return true;
}

bool SymbolProvider::parseDwarf(const std::string& path, ModuleInfo& module) {
    // DWARF parsing would require libdw
    // For now, just return true
    (void)path;
    (void)module;
    return true;
}

bool SymbolProvider::parseSymtab(const std::string& path, ModuleInfo& module) {
    // Already done in parseElf
    (void)path;
    (void)module;
    return true;
}

bool SymbolProvider::parseDynsym(const std::string& path, ModuleInfo& module) {
    // Already done in parseElf
    (void)path;
    (void)module;
    return true;
}

}
