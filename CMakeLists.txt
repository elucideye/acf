# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.3)

#########################
### CMAKE_MODULE_PATH ###
#########################

set(drishti_upload_modules "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/cmake/Modules")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")
list(APPEND CMAKE_MODULE_PATH "${drishti_upload_modules}")

############################
### HunterGate and cache ###
############################

# The drishti-upload module provides a common hunter config with
# a corresponding binary server cache.  We use this for fast CI
# builds by default.  It can be disabled with this option in favor
# of a local cmake/Hunter/config.cmake
option(ACF_USE_DRISHTI_CACHE "Use the drishti-upload config and cache" ON)

if(EXISTS "${drishti_upload_modules}" AND ACF_USE_DRISHTI_CACHE)
  # drishti specific hunter cache pacakges:
  set(
      HUNTER_CACHE_SERVERS
      "https://github.com/elucideye/hunter-cache"
      CACHE
      STRING
      "Hunter cache servers"
  )
  include(drishti_set_hunter_gate)
  drishti_set_hunter_version(ACF_HUNTER_GATE_URL ACF_HUNTER_GATE_SHA1)
  set(DRISHTI_UPLOAD_IGNORE_SUBMODULES YES)
  set(ACF_HUNTER_CONFIG "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/config.cmake")
else()
  # Release archive will not contain submodule 'drishti-upload'.
  # It's a valid case when archive used in Hunter.
  # URL/SHA1 arguments of HunterGate call will not be used while building
  # in this situation. One can also set ACF_USE_DRISHTI_CACHE=OFF in order
  # to build with recent hunter defaults.
  set(ACF_HUNTER_GATE_URL "https://github.com/ruslo/hunter/archive/v0.19.161.tar.gz")
  set(ACF_HUNTER_GATE_SHA1 "9af482b6a54875bbbb221274f7290324dff29919")
  set(ACF_HUNTER_CONFIG "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake")
endif()

include("cmake/HunterGate.cmake")
HunterGate(
  URL "${ACF_HUNTER_GATE_URL}"
  SHA1 "${ACF_HUNTER_GATE_SHA1}"
  FILEPATH "${ACF_HUNTER_CONFIG}"
  )

##########################
### CI Travis/Appveyor ###
##########################

string(COMPARE NOTEQUAL "$ENV{TRAVIS_TAG}" "" travis_deploy)
string(COMPARE EQUAL "$ENV{APPVEYOR_REPO_TAG}" "true" appveyor_deploy)

if(travis_deploy)
  set(version "$ENV{TRAVIS_TAG}")
elseif(appveyor_deploy)
  set(version "$ENV{APPVEYOR_REPO_TAG_NAME}")
else()
  set(version "v0.0.1")
endif()

string(REGEX REPLACE "^v" "" version "${version}")

###################
### ACF project ###
###################

project(acf VERSION ${version})

set(ACF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # for 3rd parties added by add_subdirectory
  cmake_policy(SET CMP0069 NEW)
endif()

string(COMPARE EQUAL "${CMAKE_SYSTEM_NAME}" "Linux" is_linux)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###############
### Options ###
###############

# make sure these are before first hunter_add_package() call
option(ACF_BUILD_APPS "Build applications" ON)
option(ACF_SERIALIZE_WITH_CEREAL "Serialize w/ cereal" ON) # always on
option(ACF_SERIALIZE_WITH_CVMATIO "Build with CVMATIO" ON)
option(ACF_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
option(ACF_BUILD_TESTS "Build tests" OFF)
option(ACF_BUILD_EXAMPLES "Build examples" ON)

################
#### Testing ###
################

string(COMPARE EQUAL "$ENV{TRAVIS}" "true" travis_ci)
string(COMPARE EQUAL "$ENV{APPVEYOR}" "True" appveyor_ci)
if(travis_ci OR appveyor_ci)
  set(ACF_CI TRUE)
else()
  set(ACF_CI FALSE)
endif()

if(ANDROID OR NOT ${ACF_CI})
  set(ACF_DO_GPU_TESTING TRUE)
else()
  set(ACF_DO_GPU_TESTING FALSE)
endif()

if(ACF_BUILD_TESTS)
  if(IOS AND ACF_CI)
    # do not run test on CI (TODO: remote device testing)
  else()
    enable_testing()
  endif()

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
  if(ACF_HAVE_TO_STRING)
    set(ACF_ADD_TO_STRING OFF)
  else()
    set(ACF_ADD_TO_STRING ON)
  endif()
else(ACF_USE_CEREAL)
  # This is never needed when CEREAL extensions are disabled
  set(ACF_ADD_TO_STRING OFF)
endif(ACF_SERIALIZE_WITH_CEREAL)

message("ACF_ADD_TO_STRING : ${ACF_ADD_TO_STRING}")

##############
## Project ###
##############

add_subdirectory(src)
