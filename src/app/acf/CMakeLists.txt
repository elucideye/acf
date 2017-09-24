set(test_app acf-detect)

##################
### acf-detect ###
##################

add_executable(${test_app} acf.cpp)
target_link_libraries(${test_app} acf cxxopts::cxxopts ${OpenCV_LIBS})
set_property(TARGET ${test_app} PROPERTY FOLDER "app/console")
install(TARGETS ${test_app} DESTINATION bin)

###############
### mat2cpb ###
###############

set(conv_app acf-mat2cpb)

if(ACF_SERIALIZE_WITH_CVMATIO)
  add_executable(${conv_app} mat2cpb.cpp)
  target_link_libraries(${conv_app} acf cxxopts::cxxopts ${OpenCV_LIBS})
  set_property(TARGET ${conv_app} PROPERTY FOLDER "app/console")
  install(TARGETS ${conv_app} DESTINATION bin)
endif()
