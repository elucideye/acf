set(test_app acf-pyramid)

###############
### pyramid ###
###############

add_executable(${test_app} pyramid.cpp)
target_link_libraries(${test_app} PUBLIC acf acf_util cxxopts::cxxopts cereal::cereal)
set_property(TARGET ${test_app} PROPERTY FOLDER "app/console")
install(TARGETS ${test_app} DESTINATION bin)
