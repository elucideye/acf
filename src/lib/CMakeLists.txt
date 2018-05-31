# This file creates project 'acf' for a single library
# After installation this project can be found
# by 'find_package(... CONFIG)' command:
#
#    find_package(acf CONFIG REQUIRED)
#    target_link_libraries(... acf::acf)
#
# Note that requirements propagated automatically, for example:

find_package(sugar CONFIG REQUIRED)
sugar_include(.)


### acf_util ###
add_library(acf_util ${ACF_UTIL_SRCS} ${ACF_UTIL_HDRS})
target_link_libraries(acf_util PUBLIC spdlog::spdlog ${OpenCV_LIBS})

### acf_shaders ###
add_library(acf_shaders ${ACF_GPU_SRCS} ${ACF_GPU_HDRS})
target_link_libraries(acf_shaders PUBLIC spdlog::spdlog ${OpenCV_LIBS})

### acf ###
if(ACF_BUILD_SHARED_SDK)
  set(acf_lib_type SHARED)
else()
  set(acf_lib_type STATIC)
endif()

add_library(acf ${acf_lib_type}
  ${ACF_SRCS}
  ${ACF_HDRS}
  ${ACF_HDRS_PUBLIC}
)

add_library(${PROJECT_NAME}::acf ALIAS acf)

target_link_libraries(acf
  PUBLIC ${ACF_3RDPARTY_PKG_LIBS}
  PRIVATE ${ACF_3RDPARTY_PKG_LIBS_} acf_util acf_shaders
)
target_compile_definitions(acf PUBLIC ACF_DO_HALF=1)  # half precision serialization
if(ACF_SERIALIZE_WITH_CVMATIO)
  target_compile_definitions(acf PUBLIC ACF_SERIALIZE_WITH_CVMATIO=1)
endif()

target_include_directories(acf
  PUBLIC
  "$<BUILD_INTERFACE:${ACF_ROOT_DIR}/src/lib/>"
  "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>"
)

foreach(lib acf_util acf_shaders acf)
  target_include_directories(${lib} PUBLIC "$<BUILD_INTERFACE:${ACF_ROOT_DIR}/src/lib/>")
  if(MSVC)
    target_compile_definitions(${lib} PUBLIC _USE_MATH_DEFINES) # define M_PI_2 for Visual Studio
    target_compile_definitions(${lib} PUBLIC NOMINMAX=1) # avoid std::{min,max}() conflicts
  endif()
  if(NOT ACF_HAVE_TO_STRING)
    target_compile_definitions(${lib} PUBLIC ACF_ADD_TO_STRING=1)
  endif()
endforeach()

### Generate the export header
include(GenerateExportHeader)
set(acf_export_dir "${CMAKE_BINARY_DIR}/acf")
set(acf_export_header "${acf_export_dir}/acf_export.h")
generate_export_header(acf EXPORT_FILE_NAME "${acf_export_header}")

string(COMPARE EQUAL "${CMAKE_OSX_SYSROOT}" "iphoneos" is_ios)
if(NOT is_ios)
  set_target_properties(acf PROPERTIES
    VERSION "${hunter_sandbox_VERSION}"
    SOVERSION "${hunter_sandbox_VERSION_MAJOR}"
    )
endif()

###############################################################
# Installation (https://github.com/forexample/package-example) {
###############################################################

# Layout. This works for all platforms:
#   * <prefix>/lib/cmake/<PROJECT-NAME>
#   * <prefix>/lib/
#   * <prefix>/include/
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(targets_export_name "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * targets_export_name
#   * PROJECT_NAME
configure_package_config_file(
    "${ACF_ROOT_DIR}/cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

# Targets:
#   * <prefix>/lib/libacf.a
#   * header location after install: <prefix>/include/acf/ACF.h
#   * headers can be included by C++ code `#include <acf/ACF.h>`
install(
    TARGETS acf acf_util acf_shaders
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    RUNTIME DESTINATION "bin"
    INCLUDES DESTINATION "${include_install_dir}"
)

# Headers:
#   * Source/acf/ACF.hpp -> <prefix>/include/acf/ACF.hpp
# install(
#     DIRECTORY "Source/foo"
#     DESTINATION "${include_install_dir}"
#     FILES_MATCHING PATTERN "*.hpp"
# )

# Export headers:
#   * ${CMAKE_CURRENT_BINARY_DIR}/acf_export.h -> <prefix>/include/acf_export.h
install(
  FILES
  "${acf_export_header}"
  ${ACF_HDRS_PUBLIC}
  DESTINATION "${include_install_dir}/${PROJECT_NAME}"
)

# Config
#   * <prefix>/lib/cmake/acf/acfConfig.cmake
#   * <prefix>/lib/cmake/acf/acfConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config#
#   * <prefix>/lib/cmake/acf/acfTargets.cmake
install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)

# }

# unit tests:
if(ACF_BUILD_TESTS)
    add_subdirectory(acf/ut)
endif()

