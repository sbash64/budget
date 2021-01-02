add_executable(
  sbash64-budget-tests
  main.cpp
  parse.cpp
  calculate.cpp
  usd.cpp
  print.cpp
  collect.cpp
  evaluate.cpp
  bank.cpp)
target_link_libraries(sbash64-budget-tests sbash64-testcpplite
                      sbash64-budget-lib GSL)
target_compile_options(sbash64-budget-tests PRIVATE ${SBASH64_BUDGET_WARNINGS})
