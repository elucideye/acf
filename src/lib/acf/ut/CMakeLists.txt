# Copyright (c) 2015-2017, Ruslan Baratov, David Hirvonen
# All rights reserved.

set(test_name ACFTest)
set(test_app test-acf)

set(acf_ut_srcs test-acf.cpp)

add_executable(${test_app} ${acf_ut_srcs})
set_property(TARGET ${test_app} PROPERTY FOLDER "app/tests")
target_link_libraries(${test_app} PUBLIC acf GTest::gtest ${OpenCV_LIBS})

if (ACF_BUILD_OGLES_GPGPU AND TARGET aglet::aglet)
  target_link_libraries(${test_app} PUBLIC aglet::aglet)
  target_compile_definitions(${test_app} PUBLIC ACF_DO_GPU=1)
endif()

# GTest + CTest
gauze_add_test(
  NAME ${test_name}
  COMMAND ${test_app}
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_FACES_FACE_IMAGE}>"
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_FACES_FACE_IMAGE}>"
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_ASSETS_FACE_DETECTOR}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_INRIA_DETECTOR_MAT}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_CALTECH_DETECTOR_MAT}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_PEDESTRIAN_IMAGE}>"
)
