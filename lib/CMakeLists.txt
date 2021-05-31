add_library(
  sbash64-budget-lib
  command-line.cpp
  bank.cpp
  serialization.cpp
  format.cpp
  parse.cpp
  control.cpp
  ${CMAKE_SOURCE_DIR}/entering-transaction/Controller.cpp)
set_source_files_properties(
  ${CMAKE_SOURCE_DIR}/entering-transaction/Controller.cpp
  PROPERTIES
    INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_LIST_DIR}/include/sbash64/budget/entering-transaction)
target_include_directories(sbash64-budget-lib PUBLIC include)
target_include_directories(sbash64-budget-lib PRIVATE include/sbash64/budget)
target_compile_options(sbash64-budget-lib PRIVATE ${SBASH64_BUDGET_WARNINGS})
target_compile_features(sbash64-budget-lib PUBLIC cxx_std_20)
set_target_properties(sbash64-budget-lib PROPERTIES CXX_EXTENSIONS OFF)
