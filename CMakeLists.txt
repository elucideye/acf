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
  set(ACF_HUNTER_GATE_URL "https://github.com/ruslo/hunter/archive/v0.20.28.tar.gz")
  set(ACF_HUNTER_GATE_SHA1 "8fee57e4232e35e28a88f0beafb5f2a739a3fa3c")
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
      # ******************
      # *** FUTURE USE ***
      # ******************
      #FILEPATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake"
    )
  endif()
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

# On CI platforms we won't have a usable GPU except for the Android emulator
# We set a variable here to help us make decisions about dependencies later on.
if(ANDROID OR NOT ${ACF_CI}) 
  set(ACF_HAS_GPU TRUE)
else()
  set(ACF_HAS_GPU FALSE)
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

endif()

##############
## Project ###
##############

add_subdirectory(src)
