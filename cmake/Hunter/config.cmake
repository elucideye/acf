string(COMPARE EQUAL "${CMAKE_OSX_SYSROOT}" "iphoneos" _is_ios)
if(_is_ios)
  set(opencv_cmake_args
    WITH_PROTOBUF=OFF
    BUILD_PROTOBUF=OFF
    BUILD_LIBPROTOBUF_FROM_SOURCES=NO
    BUILD_opencv_dnn=OFF
    )
  hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS ${opencv_cmake_args})
endif()
