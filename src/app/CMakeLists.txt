#############
### Boost ###
#############
#hunter_add_package(Boost COMPONENTS filesystem system)
#find_package(Boost CONFIG REQUIRED filesystem system)  

###############
### cxxopts ### :: std::regex >= gcc 4.8
###############
hunter_add_package(cxxopts)
find_package(cxxopts CONFIG REQUIRED)

add_subdirectory(acf)