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

option(SBASH64_BUDGET_ENABLE_WEB "Enable web implementation" OFF)
if(${SBASH64_BUDGET_ENABLE_WEB})
  FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp
    GIT_TAG 0.8.2)
  FetchContent_MakeAvailable(websocketpp)
  find_program(MAKE_EXE NAMES gmake nmake make)
  FetchContent_Declare(
    asio
    URL https://sourceforge.net/projects/asio/files/asio/1.18.2%20%28Stable%29/asio-1.18.2.zip/download
        CONFIGURE_COMMAND
        "./configure"
        BUILD_COMMAND
        ${MAKE_EXE})
  FetchContent_MakeAvailable(asio)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY
      https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
    GIT_TAG v3.9.1)
  FetchContent_MakeAvailable(nlohmann_json)

  add_subdirectory(web)
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
