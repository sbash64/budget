set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_executable(sbash64-budget-web main.cpp)
target_link_libraries(
  sbash64-budget-web PRIVATE sbash64-budget-lib Threads::Threads
                             nlohmann_json::nlohmann_json)
target_include_directories(
  sbash64-budget-web PRIVATE "${websocketpp_SOURCE_DIR}"
                             ${asio_SOURCE_DIR}/include)
target_compile_options(sbash64-budget-web PRIVATE "${SBASH64_BUDGET_WARNINGS}")
