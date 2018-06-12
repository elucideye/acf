# Copyright (c) 2015-2017, Ruslan Baratov, David Hirvonen
# All rights reserved.

################
### api test ### : STATIC or SHARED w/ hidden symbols
################

set(test_api_name ACFTest_api)
set(test_api_app test-acf-api)

set(acf_ut_srcs test-acf-api.cpp)

add_executable(${test_api_app} ${acf_ut_srcs})
set_property(TARGET ${test_api_app} PROPERTY FOLDER "app/tests")
target_link_libraries(${test_api_app} PUBLIC acf::acf acf_common GTest::gtest ${OpenCV_LIBS})

if (ACF_BUILD_OGLES_GPGPU AND TARGET aglet::aglet)
  target_link_libraries(${test_api_app} PUBLIC aglet::aglet)
  target_compile_definitions(${test_api_app} PUBLIC ACF_DO_GPU=1)
endif()

# GTest + CTest
gauze_add_test(
  NAME ${test_api_name}
  COMMAND ${test_api_app}
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_FACES_FACE_IMAGE}>"
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_FACES_FACE_IMAGE}>"
  "$<GAUZE_RESOURCE_FILE:${DRISHTI_ASSETS_FACE_DETECTOR}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_INRIA_DETECTOR_MAT}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_CALTECH_DETECTOR_MAT}>"
  "$<GAUZE_RESOURCE_FILE:${ACF_PEDESTRIAN_IMAGE}>"
)

if(WIN32 OR CYGWIN)
  set_property(
    TEST ${test_api_name}
    PROPERTY
    ENVIRONMENT
    "PATH=$<TARGET_FILE_DIR:acf>;$ENV{PATH}"
   )
endif()

#################
### unit test ### : this will always use static linking
#################

set(test_unit_name ACFTest_unit)
set(test_unit_app test-acf-unit)

set(acf_ut_srcs test-acf-unit.cpp ${ACF_TEST_SRCS})

add_executable(${test_unit_app} ${acf_ut_srcs})
set_property(TARGET ${test_unit_app} PROPERTY FOLDER "app/tests")
target_link_libraries(${test_unit_app} PUBLIC acf_headers acf_common GTest::gtest ${OpenCV_LIBS})

if (ACF_BUILD_OGLES_GPGPU)
  # Always link ogles_gpgpu in such cases to make sure the GPGPU test code builds.
  target_link_libraries(${test_unit_app} PUBLIC ogles_gpgpu::ogles_gpgpu)
  if(TARGET aglet::aglet)
    # Only link aglet if we are actually going to run the tests
    target_link_libraries(${test_unit_app} PUBLIC aglet::aglet)
    target_compile_definitions(${test_unit_app} PUBLIC ACF_DO_GPU=1)
    )
endif()

# GTest + CTest
gauze_add_test(
  NAME ${test_unit_name}
  COMMAND ${test_unit_app}
)
