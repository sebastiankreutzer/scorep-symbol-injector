//
// Created by sebastian on 04.01.22.
//

#include "SymbolInjector.h"

#include <string>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <cstring>
#include <map>
#include <sstream>
#include <chrono>

#include "FunctionFilter.h"

#define LOG_ERR std::cerr << "[Symbol Injector] "
#define LOG_OUT std::cout << "[Symbol Injector] "

namespace symbolinjector{

    class Timer {
        using clock = std::chrono::steady_clock;
        std::string msg;
        clock::time_point start;
    public:
        Timer(const std::string& msg) : msg(msg), start(clock::now()){
        }
        ~Timer() {
            auto stop = clock::now();
            auto diff = stop - start;
            auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0;
            LOG_OUT << msg << secs << " seconds\n";
        }
    };

    struct RemoveEnvInScope {

        explicit RemoveEnvInScope(const char* varName) : varName(varName) {
            oldVal = getenv(varName);
            if (oldVal)
                setenv(varName, "", true);
        }

        ~RemoveEnvInScope() {
            if (oldVal)
                setenv(varName, oldVal, true);
        }

    private:
        const char* varName;
        const char* oldVal;
    };


    std::string getExecPath() {
        RemoveEnvInScope removePreload("LD_PRELOAD");
        char filename[128] = {0};
        auto n = readlink("/proc/self/exe", filename, sizeof(filename) - 1);
        if (n > 0) {
            return filename;
        }
        return "";
    }

    std::vector<MemMapEntry> readMemoryMap() {
        RemoveEnvInScope removePreload("LD_PRELOAD");

        std::vector<MemMapEntry> entries;

        char buffer[256];
        FILE *memory_map = fopen("/proc/self/maps", "r");
        if (!memory_map) {
            LOG_OUT << "Could not load memory map.\n";
        }

        std::string addrRange;
        std::string perms;
        uint64_t offset;
        std::string dev;
        uint64_t inode;
        std::string path;
        while (fgets(buffer, sizeof(buffer), memory_map)) {
            std::istringstream lineStream(buffer);
            lineStream >> addrRange >> perms >> std::hex >> offset >> std::dec >> dev >> inode >> path;
            //std::cout << "Memory Map: " << addrRange << ", " << perms << ", " << offset << ", " << path <<"\n";
            if (strcmp(perms.c_str(), "r-xp") != 0) {
                continue;
            }
            if (path.find(".so") == std::string::npos) {
                continue;
            }
            uintptr_t addrBegin = std::stoul(addrRange.substr(0, addrRange.find('-')), nullptr, 16);
            //std::cout << std::hex << addrBegin << '\n';
            entries.push_back({path, addrBegin, offset});
        }
        return entries;
    }



    SymbolTable loadSymbolTable(const std::string& object_file) {
        // Need to disable LD_PRELOAD, otherwise this library will be loaded in popen call.
        RemoveEnvInScope removePreload("LD_PRELOAD");

        std::string command = "nm --defined-only ";
        if (object_file.find(".so") != std::string::npos) {
            command += "-D ";
        }
        command += object_file;

        char buffer[256] = {0};
        FILE *output = popen(command.c_str(), "r");
        if (!output) {
            LOG_ERR << "Unable to execute nm to resolve symbol names.\n";
            return {};
        }

        SymbolTable table;

        uintptr_t addr;
        std::string symType;
        std::string symName;

        while (fgets(buffer, sizeof(buffer), output)) {
            std::istringstream line(buffer);
            if (buffer[0] != '0') {
                continue;
            }
            line >> std::hex >> addr;
            line >> symType;
            line >> symName;
            table[addr] = symName;
        }
        pclose(output);

        return table;

    }

    SymbolRetriever::SymbolRetriever(std::string execFile) : execFile(std::move(execFile)) {
    }

    bool SymbolRetriever::run() {
        loadSymTables();
        return true;
    }

    bool SymbolRetriever::loadSymTables() {

        addrToSymTable.clear();

        // Load symbols from main executable
        auto execSyms = loadSymbolTable(execFile);
        MappedSymTable execTable{std::move(execSyms), {execFile, 0, 0}};
        addrToSymTable[0] = execTable;

        // Load symbols from shared libs
        auto memMap = readMemoryMap();
        for (auto &entry: memMap) {
            auto &filename = entry.path;
            auto table = loadSymbolTable(filename);
            if (table.empty()) {
                LOG_ERR << "Could not load symbols from " << filename << "\n";
                continue;
            }

            //std::cout << "Loaded " << table.size() << " symbols from " << filename << "\n";
            //std::cout << "Starting address: " << entry.addrBegin << "\n";
            MappedSymTable mappedTable{std::move(table), entry};
            addrToSymTable[entry.addrBegin] = mappedTable;
            //        symTables.emplace_back(filename, std::move(mappedTable));
        }
        return true;
    }

    uint64_t mapAddrToProc(uint64_t addrInLib, const MappedSymTable& table) {
        return table.memMap.addrBegin + addrInLib - table.memMap.offset;
    }

    RunBeforeMain::RunBeforeMain() {
        Timer timer("Symbol lookup and registration took ");
        auto execPath = getExecPath();
        auto execFilename = execPath.substr(execPath.find_last_of('/') + 1);
        if (auto val = getenv("SCOREP_EXECUTABLE"); val) {
            // TODO: SCOREP_EXECUTABLE usually contains the full path
            if (strcmp(val, execFilename.c_str()) != 0) {
                return;
            }
            LOG_OUT << "Executable will be tracked: SCOREP_EXECUTABLE is set to " << val << " and matches the current executable filename " << execFilename << "\n";
        } else {
            LOG_OUT << "Please set SCOREP_EXECUTABLE=" << execFilename << ", if you want to track calls in this application.\n";
            return;
        }
        //std::cout << "Retrieving symbols for executable " << execPath << "\n";

        // Load ScoreP filter file
        FunctionFilter filter;
        auto filterEnv = std::getenv("SCOREP_FILTERING_FILE");
        if (filterEnv) {
            bool success = readScorePFilterFile(filter, filterEnv);
            if (!success)  {
                LOG_ERR << "Unable to read Score-P filtering file from " << filterEnv << "\n";
            }
        }

        // Initializing ScoreP
        SCOREP_InitMeasurement();

        SymbolRetriever symRetriever(execFilename);
        symRetriever.run();

        auto& symTables = symRetriever.getMappedSymTables();

        size_t numFound = 0;
        size_t numInserted = 0;

        for (auto&& [startAddr, table] : symTables) {
            for (auto&& [addr, symName] : table.table) {
                auto addrInProc = mapAddrToProc(addr, table);
                // TODO: Demangling
                if (filter.accepts(symName)) {
                    scorep_compiler_hash_put(addrInProc, symName.c_str(), symName.c_str(), "", 0);
                    ++numInserted;
                }
                ++numFound;
            }
        }

        LOG_OUT << "Symbols found: " << numFound << "\n";
        LOG_OUT << "Symbols registered: " << numInserted << "\n";

    }
}

namespace {
    symbolinjector::RunBeforeMain initializer;
}