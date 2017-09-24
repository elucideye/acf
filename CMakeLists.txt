# Copyright (c) 2015-2017, David Hirvonen, Ruslan Baratov
# All rights reserved.

cmake_minimum_required(VERSION 3.3)

### Hunter snapshot that will be used ###
include("cmake/HunterGate.cmake")

HunterGate(
  URL "https://github.com/ruslo/hunter/archive/v0.19.109.tar.gz"
  SHA1 "2c416fea5f66429ffb4412ce5b8d0c6799ec44d2"
  FILEPATH "${CMAKE_CURRENT_LIST_DIR}/cmake/Hunter/config.cmake"    
)

set(ACF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}")

project(acf VERSION 0.0.1)

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # for 3rd parties added by add_subdirectory
  cmake_policy(SET CMP0069 NEW)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# make sure these are before first hunter_add_package() call
option(ACF_BUILD_TESTS "Build tests" ON)
option(ACF_BUILD_APPS "Build applications" ON)
option(ACF_SERIALIZE_WITH_CEREAL "Serialize w/ cereal" ON) # hunter update
option(ACF_SERIALIZE_WITH_CVMATIO "Build with CVMATIO" ON)
option(ACF_BUILD_OGLES_GPGPU "Build with OGLES_GPGPU" ON)

# GTest
if(ACF_BUILD_TESTS)
  enable_testing()
  hunter_add_package(GTest)
  find_package(GTest CONFIG REQUIRED)
endif()

##############
## Project ###
##############

add_subdirectory(src)
