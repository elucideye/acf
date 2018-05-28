# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.9) # CMAKE_INTERPROCEDURAL_OPTIMIZATION

# By default, the source code for all managed dependencies will be removed after
# building and installing to the cache.  This behavior is consistent with most
# installed libraries (i.e., /usr/local/lib/*), but it does prevent stepping
# through the dependencies in a debugger in cases where a problem is not
# contained within the acf project sources.  In such cases, you can set
# HUNTER_KEEP_PACKAGE_SOURCES=ON from the command line during the project
# configuration and the source will be left for all packages when they are
# created.  This setting must be used before a package is installed -- it
# won't be applied retroactively to installed packages.  In order to re-install
# a package with sources you can always remove the cache
# (i.e., rm -rf ${HOME}/.hunter) or, less drastically you can modify the
# CONFIG-ID of an installed package to trigger the configuration and
# installation steps.  This can be done by modifying the input CMAKE_ARGS
# list in a hunter_config() call.  In the following example KEEP_SOURCES=1
# is  added to trigger a re-installation:
#
#   hunter_config(foo VERSION ${HUNTER_foo_VERSION} CMAKE_ARGS KEEP_SOURCES=1)
#
# The HUNTER_KEEP_PACKAGE_SOURCES development feature is described here:
#
# In order to support stepping through package sources you will also have to
# make sure that debug versions of the packages are installed.  This will
# happen by default, but will not happen if you specify a release only build
# using HUNTER_CONFIGURATION_TYPES=Release

# https://docs.hunter.sh/en/latest/reference/user-variables.html#hunter-keep-package-sources
option(HUNTER_KEEP_PACKAGE_SOURCES "Keep installed package sources for debugging (caveat...)" ON)

#########################
### CMAKE_MODULE_PATH ###
#########################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

############################
### HunterGate and cache ###
############################

option(ACF_USE_DRISHTI_UPLOAD "Use drishti upload configuration" OFF)
option(DRISHTI_UPLOAD_IGNORE_SUBMODULES "Ignore submodules" ON) # override
set(drishti_upload_init "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/init.cmake") # cache upload

if(ACF_USE_DRISHTI_UPLOAD)
  if(EXISTS "${drishti_upload_init}")
    # CI: Locked CI build configuration
    include("cmake/HunterGate.cmake") # HUNTER_ENABLED
    include("${drishti_upload_init}")
  else()
    # FAILURE:
    message(
      FATAL_ERROR
      "File does not exist:\n"
      "  ${drishti_upload_init}\n"
      "Run this command if submodule is not initialized:\n"
      "  git submodule update --init --recursive ."
  )
  endif()
else()
  set(ACF_HUNTER_GATE_URL "https://github.com/ruslo/hunter/archive/v0.21.19.tar.gz")
  set(ACF_HUNTER_GATE_SHA1 "5ead1e069b437930d0de8a21824b20fb52b37b50")
  if(HUNTER_ENABLED AND NOT ACF_IS_REPO_BUILD)
    # DEPENDENCY: if(HUNTER_ENABLE): we are being used as a dependency from
    # another project and we will use the config.cmake from the parent project.
    message(STATUS "ACF Hunter package build detected")
    include("cmake/HunterGate.cmake") # HUNTER_ENABLED
    HunterGate(
      URL "${ACF_HUNTER_GATE_URL}"
      SHA1 "${ACF_HUNTER_GATE_SHA1}"
    )
  else()
    # LOCAL: This is a local repository build and we are relying on a local
    # config (instead of CI friendly drishti-upload) that is closer to the
    # current default versions in ${HUNTER_ROOT}/cmake/configs/default.cmake
    # *IF* we want to enable local project defaults then they would go in
    # "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake"
    message(STATUS "ACF in-repository build detected")
    set(ACF_IS_REPO_BUILD TRUE CACHE STRING "Is a build from the local repository")
    include("cmake/HunterGate.cmake") # HUNTER_ENABLED
    HunterGate(
      URL "${ACF_HUNTER_GATE_URL}"
      SHA1 "${ACF_HUNTER_GATE_SHA1}"
      FILEPATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake"
    )
  endif()
endif()

###################
### ACF project ###
###################

# See https://github.com/hunter-packages/check_ci_tag when changing VERSION
project(acf VERSION 0.1.12)

set(ACF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # for 3rd parties added by add_subdirectory
  cmake_policy(SET CMP0069 NEW)
endif()

### RPATH ###
if (APPLE)
  set(ACF_ORIGIN "@loader_path")
else()
  set(ACF_ORIGIN "$ORIGIN")
endif()
set(CMAKE_INSTALL_RPATH "${ACF_ORIGIN}/../lib" "${ACF_ORIGIN}")


string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###############
### Options ###
###############

# make sure these are before first hunter_add_package() call
option(ACF_BUILD_SHARED_SDK "Build ACF as a shared library" OFF)
option(ACF_BUILD_APPS "Build applications" ON)
option(ACF_SERIALIZE_WITH_CEREAL "Serialize w/ cereal" ON) # always on
option(ACF_SERIALIZE_WITH_CVMATIO "Build with CVMATIO" ON)
option(ACF_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
option(ACF_BUILD_TESTS "Build tests" OFF)
option(ACF_BUILD_EXAMPLES "Build examples" ON)
option(ACF_OPENGL_ES3 "Use OpenGL ES 3.0 (and compatible)" OFF)

####################
## Version check ###
####################

# https://github.com/hunter-packages/check_ci_tag
hunter_add_package(check_ci_tag)
find_package(check_ci_tag CONFIG REQUIRED)
check_ci_tag()

#######################
### Packaging/CPack ###
#######################
# see https://github.com/forexample/github-binary-release/blob/master/CMakeLists.txt

include(InstallRequiredSystemLibraries)
include("cmake/pack.cmake") # includes Cpack

string(COMPARE NOTEQUAL "$ENV{TRAVIS_TAG}" "" travis_deploy)
string(COMPARE EQUAL "$ENV{APPVEYOR_REPO_TAG}" "true" appveyor_deploy)

if(travis_deploy OR appveyor_deploy)
  string(COMPARE EQUAL "$ENV{CONFIG}" "Debug" debug_build)
  if(debug_build)
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-$ENV{TOOLCHAIN}-Debug")
  else()
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-$ENV{TOOLCHAIN}")
  endif()
endif()

################
#### Testing ###
################

option(ACF_HAS_GPU "Drishti has GPU" ON)

if(ACF_BUILD_TESTS)
  enable_testing()
  
  hunter_add_package(gauze)
  find_package(gauze CONFIG REQUIRED)

  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)

  ###################
  ### Test models ###
  ###################
  hunter_add_package(drishti_assets)
  find_package(drishti_assets CONFIG REQUIRED)

  set(DRISHTI_ASSETS_VARS
    DRISHTI_ASSETS_FACE_DETECTOR
    DRISHTI_ASSETS_FACE_DETECTOR_MEAN
    )

  foreach(model ${DRISHTI_ASSETS_VARS})
    # Override assets using corresponding environment variables if present:
    if(DEFINED ENV{${model}})
      message("RESET ${model} = $ENV{${model}}")
      unset(${model} CACHE)
      set(${model} $ENV{${model}})
    endif()
  endforeach()

  #################
  ### Test data ###
  #################
  hunter_add_package(drishti_faces)
  find_package(drishti_faces CONFIG REQUIRED)

endif()

if(ACF_SERIALIZE_WITH_CEREAL)
  #### std::to_string ####
  try_compile(ACF_HAVE_TO_STRING "${CMAKE_BINARY_DIR}/compile_tests" "${PROJECT_SOURCE_DIR}/cmake/to_string.cpp")
endif(ACF_SERIALIZE_WITH_CEREAL)

################
## TEST DATA ###
################

# Only download test data if tests are enabled and we are running
# a recent enough version of Hunter to support hunter_private_data()
if(ACF_BUILD_TESTS)

  if(NOT COMMAND hunter_private_data)
    message(
      FATAL_ERROR
      "hunter_private_data(): Function is not defined.\n"
      "Please update to a newer version of Hunter (>= v0.20.3)"
    )
  endif()

  set(acf_release_url "https://github.com/elucideye/acf/releases/download/v0.0.0")

  hunter_private_data(
    URL "${acf_release_url}/AcfCaltech.Detector.mat"
    SHA1 "c19bfd560d66e748a88e48885e944ce6ba55c77a"
    FILE "AcfCaltech.Detector.mat"
    LOCATION ACF_CALTECH_DETECTOR_MAT
  )
  message("ACF_CALTECH_DETECTOR_MAT = ${ACF_CALTECH_DETECTOR_MAT}")

  hunter_private_data(
    URL "${acf_release_url}/AcfInriaDetector.mat"
    SHA1 "eb1d1bf0a39e09766ce055640659b5a40e543395"
    FILE "AcfInriaDetector.mat"
    LOCATION ACF_INRIA_DETECTOR_MAT
    )
  message("ACF_INRIA_DETECTOR_MAT = ${ACF_INRIA_DETECTOR_MAT}")

  hunter_private_data(
    URL "${acf_release_url}/city_crossing_pedestrian_crossing_pedestrians_people_road_street-915416.png"
    SHA1 "043962311fe78bd434be0bada127f8fee3b05072"
    FILE "city_crossing_pedestrian_crossing_pedestrians_people_road_street-915416.png"
    LOCATION ACF_PEDESTRIAN_IMAGE
  )
  message("ACF_PEDESTRIAN_IMAGE = ${ACF_PEDESTRIAN_IMAGE}")

  hunter_private_data(
    URL "${acf_release_url}/dodecagon.png"
    SHA1 "1b5149cdf40611426de1e1951fb3d9c947b9f080"
    FILE "dodecagon.png"
    LOCATION ACF_DODECAGON_IMAGE
  )
  message("ACF_DODECAGON_IMAGE = ${ACF_DODECAGON_IMAGE}")

endif()

##############
## Project ###
##############

add_subdirectory(src)
