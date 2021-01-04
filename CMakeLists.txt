include(FetchContent)
FetchContent_Declare(
  GSL
  GIT_REPOSITORY https://github.com/microsoft/GSL
  GIT_TAG v3.1.0)
FetchContent_MakeAvailable(GSL)

add_subdirectory(lib)
add_executable(sbash64-budget main.cpp)
target_link_libraries(sbash64-budget sbash64-budget-lib)

option(SBASH64_BUDGET_ENABLE_TESTS "Enable tests" OFF)
if(${SBASH64_BUDGET_ENABLE_TESTS})
  FetchContent_Declare(
    testcpplite
    GIT_REPOSITORY https://github.com/sbash64/testcpplite
    GIT_TAG v2.1.1)
  FetchContent_MakeAvailable(testcpplite)

  enable_testing()
  add_subdirectory(test)
endif()
