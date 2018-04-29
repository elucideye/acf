if(IOS OR ANDROID)
  # local workaround for protobuf compiler crash with Xcode 8.1
  # see https://github.com/elucideye/acf/issues/41
  set(opencv_cmake_args
    WITH_PROTOBUF=OFF
    BUILD_PROTOBUF=OFF
    BUILD_LIBPROTOBUF_FROM_SOURCES=NO
    BUILD_opencv_dnn=OFF
     
    WITH_JASPER=OFF
    BUILD_JASPER=OFF
  )
  hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS ${opencv_cmake_args})  
endif()

### ogles_gpgpu ###
set(ogles_gpgpu_cmake_args
  OGLES_GPGPU_VERBOSE=OFF
  OGLES_GPGPU_OPENGL_ES3=${ACF_OPENGL_ES3}
)
hunter_config(ogles_gpgpu VERSION ${HUNTER_ogles_gpgpu_VERSION} CMAKE_ARGS ${ogles_gpgpu_cmake_args})


