###############
### cxxopts ### :: std::regex >= gcc 4.8
###############
hunter_add_package(cxxopts)
find_package(cxxopts CONFIG REQUIRED)

add_subdirectory(acf)

if (ACF_BUILD_OGLES_GPGPU AND TARGET aglet::aglet)
  add_subdirectory(pipeline)
endif()
