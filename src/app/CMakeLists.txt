###############
### cxxopts ### :: std::regex >= gcc 4.8
###############
hunter_add_package(cxxopts)
find_package(cxxopts CONFIG REQUIRED)

add_subdirectory(acf)

add_subdirectory(pipeline)
