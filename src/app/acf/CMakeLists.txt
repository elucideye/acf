set(test_app acf-detect)

##################
### acf-detect ###
##################

if(ACF_BUILD_OGLES_GPGPU AND TARGET aglet::aglet)
  set(ACF_DO_GPU TRUE)
else()
  set(ACF_DO_GPU FALSE)
endif()

# Define list of sources, add gpu files as needed
set(acf_srcs acf.cpp)
if(${ACF_DO_GPU})
  list(APPEND acf_srcs GLDetector.h GLDetector.cpp)
endif()

add_executable(${test_app} ${acf_srcs})
target_link_libraries(${test_app} PUBLIC acf::acf acf_common cxxopts::cxxopts)

if(${ACF_DO_GPU})
  target_include_directories(${test_app} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../>")
  target_link_libraries(${test_app} PUBLIC aglet::aglet)
  target_compile_definitions(${test_app} PUBLIC ACF_DO_GPU=1)
endif()

set_property(TARGET ${test_app} PROPERTY FOLDER "app/console")
install(TARGETS ${test_app} DESTINATION bin)

set_target_properties(
  ${test_app}
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/plist.in" # file sharing
  XCODE_ATTRIBUTE_PRODUCT_NAME "${test_app}"
  XCODE_ATTRIBUTE_BUNDLE_IDENTIFIER "com.elucideye.acf.${test_app}"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.elucideye.acf.${test_app}"
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2" # iPhone/iPad
)

###############
### mat2cpb ###
###############

set(conv_app acf-mat2cpb)

if(ACF_SERIALIZE_WITH_CVMATIO)
  add_executable(${conv_app} mat2cpb.cpp)
  target_link_libraries(${conv_app} PUBLIC acf::acf acf_common cxxopts::cxxopts cereal::cereal)
  set_property(TARGET ${conv_app} PROPERTY FOLDER "app/console")
  install(TARGETS ${conv_app} DESTINATION bin)
endif()
