add_executable(
  sbash64-budget-tests
  main.cpp
  parse.cpp
  usd.cpp
  print.cpp
  evaluate.cpp
  bank.cpp
  account.cpp
  file.cpp)
target_link_libraries(sbash64-budget-tests sbash64-testcpplite
                      sbash64-budget-lib GSL)
target_compile_options(sbash64-budget-tests PRIVATE ${SBASH64_BUDGET_WARNINGS})
add_test(NAME sbash64-budget-tests COMMAND sbash64-budget-tests)
