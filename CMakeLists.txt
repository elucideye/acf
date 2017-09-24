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

# drishti specific hunter cache pacakges:
list(APPEND HUNTER_CACHE_SERVERS "https://github.com/elucideye/hunter-cache")

if(EXISTS "${drishti_upload_modules}")
  include(drishti_set_hunter_gate)
  drishti_set_hunter_version(DRISHTI_HUNTER_GATE_URL DRISHTI_HUNTER_GATE_SHA1)
else()
  # Release archive will not contain submodule 'drishti-upload'.
  # It's a valid case when archive used in Hunter.
  # URL/SHA1 arguments of HunterGate call will not be used while building
  # in this situation.
  set(DRISHTI_HUNTER_GATE_URL)
  set(DRISHTI_HUNTER_GATE_SHA1)
endif()

include("cmake/HunterGate.cmake")
HunterGate(
  URL "${DRISHTI_HUNTER_GATE_URL}"
  SHA1 "${DRISHTI_HUNTER_GATE_SHA1}"
  FILEPATH "${CMAKE_CURRENT_LIST_DIR}/drishti-upload/config.cmake"
  )

set(DRISHTI_UPLOAD_IGNORE_SUBMODULES YES)

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
if(IOS OR ANDROID)
  set(DRISHTI_IS_MOBILE TRUE)
else()
  set(DRISHTI_IS_MOBILE FALSE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###############
### Options ###
###############

# make sure these are before first hunter_add_package() call
option(ACF_BUILD_APPS "Build applications" ON)
option(ACF_SERIALIZE_WITH_CEREAL "Serialize w/ cereal" ON) # hunter update
option(ACF_SERIALIZE_WITH_CVMATIO "Build with CVMATIO" ON)
option(ACF_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)
option(ACF_BUILD_TESTS "Build tests" ON)

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

##############
## Project ###
##############

add_subdirectory(src)
