
add_library(symbol_injector_static STATIC SymbolInjector.cpp)
target_link_libraries(symbol_injector_static PRIVATE ${scorep_mgmt} ${scorep_measurement})

add_library(symbol_injector SHARED SymbolInjector.cpp)
target_link_libraries(symbol_injector PRIVATE ${scorep_mgmt} ${scorep_measurement})

