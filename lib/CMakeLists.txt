add_library(sbash64-budget-lib parse.cpp print.cpp evaluate.cpp bank.cpp
                               account.cpp serialization.cpp)
target_include_directories(sbash64-budget-lib PUBLIC include)
target_include_directories(sbash64-budget-lib PRIVATE include/sbash64/budget)
target_compile_features(sbash64-budget-lib PUBLIC cxx_std_17)
set_target_properties(sbash64-budget-lib PROPERTIES CXX_EXTENSIONS OFF)
