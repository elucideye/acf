# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.3)

#########################
### CMAKE_MODULE_PATH ###
#########################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Modules")

############################
### HunterGate and cache ###
############################

include("cmake/HunterGate.cmake")

set(drishti_upload_init "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/init.cmake")

if(EXISTS "${drishti_upload_init}")
  option(DRISHTI_UPLOAD_IGNORE_SUBMODULES "Ignore submodules" ON)
  include("${drishti_upload_init}")
elseif(HUNTER_ENABLED)
  # Building as a Hunter dependency. This settings will be inherited from
  # parent project and was not used in fact.
  HunterGate(
      URL "https://github.com/ruslo/hunter/archive/v0.19.237.tar.gz"
      SHA1 "b64ad42b7ef398a7332f5f4a98cf7bbfd38ca8d0"
  )
else()
  message(
      FATAL_ERROR
      "File not exists:\n"
      "  ${drishti_upload_init}\n"
      "Run this command if submodule is not initialized:\n"
      "  git submodule update --init --recursive ."
  )
endif()

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
