add_executable(sbash64-budget-tests main.cpp parse.cpp)
target_link_libraries(sbash64-budget-tests sbash64-testcpplite
                      sbash64-budget-lib)
target_compile_options(sbash64-budget-tests PRIVATE ${SBASH64_BUDGET_WARNINGS})
