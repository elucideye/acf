if(IOS)
  # local workaround for protobuf compiler crash with Xcode 8.1
  # see https://github.com/elucideye/acf/issues/41
  set(opencv_cmake_args
    WITH_PROTOBUF=OFF
    BUILD_PROTOBUF=OFF
    BUILD_LIBPROTOBUF_FROM_SOURCES=NO
    BUILD_opencv_dnn=OFF
    )
  hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS ${opencv_cmake_args})
endif()
