cmake_minimum_required(VERSION 3.19)
cmake_policy(SET CMP0048 NEW)
project(sbash64-budget LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  GSL
  GIT_REPOSITORY https://github.com/microsoft/GSL
  GIT_TAG v3.1.0)
FetchContent_MakeAvailable(GSL)

add_subdirectory(lib)
add_executable(sbash64-budget main.cpp)
target_link_libraries(sbash64-budget sbash64-budget-lib)
set_target_properties(sbash64-budget PROPERTIES CXX_EXTENSIONS OFF)

option(SBASH64_BUDGET_ENABLE_GTK "Enable GTK implementation" OFF)
if(${SBASH64_BUDGET_ENABLE_GTK})
  add_subdirectory(gtk)
endif()

option(SBASH64_BUDGET_ENABLE_TESTS "Enable tests" OFF)
if(${SBASH64_BUDGET_ENABLE_TESTS})
  FetchContent_Declare(
    testcpplite
    GIT_REPOSITORY https://github.com/sbash64/testcpplite
    GIT_TAG v3.0.0)
  FetchContent_MakeAvailable(testcpplite)

  enable_testing()
  add_subdirectory(test)
endif()
