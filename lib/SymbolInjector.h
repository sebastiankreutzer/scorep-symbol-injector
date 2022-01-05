//
// Created by sebastian on 04.01.22.
//

#ifndef SCOREP_SYMBOL_INJECTOR_SYMBOLINJECTOR_H
#define SCOREP_SYMBOL_INJECTOR_SYMBOLINJECTOR_H


#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>

extern "C" {
typedef uint32_t SCOREP_LineNo;
struct scorep_compiler_hash_node;

scorep_compiler_hash_node* scorep_compiler_hash_put(uint64_t key, const char* region_name_mangled, const char* region_name_demangled, const char* file_name, SCOREP_LineNo line_no_begin);

void SCOREP_InitMeasurement();
};

namespace symbolinjector {

    using SymbolTable = std::map<std::uintptr_t, std::string>;

    struct MemMapEntry {
        std::string path;
        uintptr_t addrBegin;
        uint64_t offset;
    };

    struct MappedSymTable {
        SymbolTable table;
        MemMapEntry memMap;
    };



    struct RunBeforeMain {
        RunBeforeMain();
    };

    class SymbolRetriever {
        std::string execFile;

        std::map<uintptr_t, MappedSymTable> addrToSymTable;
        std::vector<std::pair<std::string, MappedSymTable>> symTables;

        bool loadSymTables();

    public:
        SymbolRetriever(std::string execFile);

        bool run();

        const std::map<uintptr_t, MappedSymTable>& getMappedSymTables() const {
            return addrToSymTable;
        }

    };

}

#endif //SCOREP_SYMBOL_INJECTOR_SYMBOLINJECTOR_H
