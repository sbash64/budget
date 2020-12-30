add_library(sbash64-budget-lib parse.cpp calculate.cpp print.cpp collect.cpp
                               evaluate.cpp)
target_include_directories(sbash64-budget-lib PUBLIC include)
target_include_directories(sbash64-budget-lib PRIVATE include/sbash64/budget)
target_compile_features(sbash64-budget-lib PUBLIC cxx_std_17)
