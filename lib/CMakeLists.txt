
set(LIB_SOURCES SymbolInjector.cpp FunctionFilter.cpp)

add_library(symbol_injector_static STATIC ${LIB_SOURCES})
target_link_libraries(symbol_injector_static PRIVATE ${scorep_mgmt} ${scorep_measurement})

add_library(symbol_injector SHARED ${LIB_SOURCES})
target_link_libraries(symbol_injector PRIVATE ${scorep_mgmt} ${scorep_measurement})

