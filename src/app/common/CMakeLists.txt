sugar_include(.)

hunter_add_package(spdlog)
find_package(spdlog CONFIG REQUIRED)

add_library(acf_common ${ACF_COMMON_SRCS} ${ACF_COMMON_HDRS})
target_link_libraries(acf_common PUBLIC spdlog::spdlog)
target_include_directories(acf_common PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/..")
if(MSVC)
  target_compile_definitions(acf_common PUBLIC _USE_MATH_DEFINES) # define M_PI_2 for Visual Studio
  target_compile_definitions(acf_common PUBLIC NOMINMAX=1) # avoid std::{min,max}() conflicts
endif()
