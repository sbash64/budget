add_library(sbash64-budget-lib bank.cpp account.cpp serialization.cpp
                               format.cpp parse.cpp)
target_include_directories(sbash64-budget-lib PUBLIC include)
target_include_directories(sbash64-budget-lib PRIVATE include/sbash64/budget)
target_compile_options(sbash64-budget-lib PRIVATE ${SBASH64_BUDGET_WARNINGS})
target_compile_features(sbash64-budget-lib PUBLIC cxx_std_20)
set_target_properties(sbash64-budget-lib PROPERTIES CXX_EXTENSIONS OFF)
