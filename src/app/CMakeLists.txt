if(NOT TARGET acf::acf)

  set(ACF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

  cmake_minimum_required(VERSION 3.9) # CMAKE_INTERPROCEDURAL_OPTIMIZATION

  # https://docs.hunter.sh/en/latest/reference/user-variables.html#hunter-keep-package-sources
  option(HUNTER_KEEP_PACKAGE_SOURCES "Keep installed package sources for debugging (caveat...)" OFF)

  list(APPEND CMAKE_MODULE_PATH "${ACF_ROOT_DIR}/cmake/Modules")

  # Test SDK samples using the Hunter packages (release based)
  include("${ACF_ROOT_DIR}/cmake/default_options.cmake")
  include("${ACF_ROOT_DIR}/cmake/HunterGate.cmake")
  
  HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.22.8.tar.gz"
    SHA1 "e7fe3f0b61b1b4844d440ec0a02ac73ddc5f0bb2"
    FILEPATH "${ACF_ROOT_DIR}/cmake/Hunter/config.cmake"
  )

  project(acf-apps VERSION 0.0.1)

  hunter_add_package(acf)
  find_package(acf CONFIG REQUIRED)

  hunter_add_package(cxxopts)
  find_package(cxxopts CONFIG REQUIRED)

  hunter_add_package(spdlog)
  find_package(spdlog CONFIG REQUIRED)

  hunter_add_package(sugar)
  find_package(sugar CONFIG REQUIRED)

  # Create acf_util lib
  sugar_include("${ACF_ROOT_DIR}/src/lib/util")  
  add_library(acf_util ${ACF_UTIL_SRCS} ${ACF_UTIL_HDRS})
  target_link_libraries(acf_util PUBLIC spdlog::spdlog)
  target_include_directories(acf_util
    PUBLIC
    "$<BUILD_INTERFACE:${ACF_ROOT_DIR}/src/lib>"
  )

  if(ACF_BUILD_OGLES_GPGPU AND ACF_HAS_GPU)
    hunter_add_package(aglet)
    find_package(aglet CONFIG REQUIRED)
  endif()

else()

  hunter_add_package(cxxopts)
  find_package(cxxopts CONFIG REQUIRED)

endif()

add_subdirectory(acf)

if (ACF_BUILD_OGLES_GPGPU AND ACF_HAS_GPU)
  add_subdirectory(pipeline)
endif()

add_subdirectory(pyramid)
