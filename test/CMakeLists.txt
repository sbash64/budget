add_executable(
  sbash64-budget-tests
  main.cpp
  parse.cpp
  usd.cpp
  format.cpp
  budget.cpp
  account.cpp
  stream.cpp
  transaction.cpp
  presentation.cpp)
target_link_libraries(sbash64-budget-tests sbash64-testcpplite
                      sbash64-budget-lib GSL)
target_compile_options(sbash64-budget-tests PRIVATE ${SBASH64_BUDGET_WARNINGS})
set_target_properties(sbash64-budget-tests PROPERTIES CXX_EXTENSIONS OFF)
add_test(NAME sbash64-budget-tests COMMAND sbash64-budget-tests)
