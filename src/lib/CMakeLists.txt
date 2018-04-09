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

add_library(acf ${ACF_SRCS} ${ACF_HDRS} ${ACF_HDRS_PUBLIC})
target_link_libraries(acf
  PUBLIC ${ACF_3RDPARTY_PKG_LIBS}
  PRIVATE ${ACF_3RDPARTY_PKG_LIBS_}
)
target_compile_definitions(acf PUBLIC _USE_MATH_DEFINES)  # define M_PI_2 for Visual Studio
target_compile_definitions(acf PRIVATE ACF_DO_HALF=1)  # half precision serialization
if(ACF_SERIALIZE_WITH_CVMATIO)
  target_compile_definitions(acf PRIVATE ACF_SERIALIZE_WITH_CVMATIO=1)
endif()
if(ACF_ADD_TO_STRING)
  target_compile_definitions(acf PRIVATE ACF_ADD_TO_STRING=1)
endif()

### Generate the export header
include(GenerateExportHeader)
set(acf_export_dir "${CMAKE_BINARY_DIR}/acf")
set(acf_export_header "${acf_export_dir}/acf_export.h")
generate_export_header(acf EXPORT_FILE_NAME "${acf_export_header}")

target_include_directories(acf
  PUBLIC
  "$<BUILD_INTERFACE:${ACF_ROOT_DIR}/src/lib/>"
  "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>"
  )

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
    TARGETS acf
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

