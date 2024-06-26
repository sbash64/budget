cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0135 NEW)
project(sbash64-budget LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
  GSL
  GIT_REPOSITORY https://github.com/microsoft/GSL
  GIT_TAG v4.0.0)
FetchContent_MakeAvailable(GSL)

add_subdirectory(lib)

option(SBASH64_BUDGET_ENABLE_WEB "Enable web implementation" OFF)
if(${SBASH64_BUDGET_ENABLE_WEB})
  FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/sbash64/websocketpp
    GIT_TAG 6503e01b34f3f73265bf68c8b7dd88b38e77297d)
  FetchContent_MakeAvailable(websocketpp)
  FetchContent_Declare(
    asio
    URL https://sourceforge.net/projects/asio/files/asio/1.28.0%20%28Stable%29/asio-1.28.0.zip/download
  )
  FetchContent_MakeAvailable(asio)
  FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
  )
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
