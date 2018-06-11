### aglet ###
set(aglet_cmake_args
  AGLET_OPENGL_ES3=${ACF_OPENGL_ES3}
)
hunter_config(ogles_gpgpu VERSION ${HUNTER_ogles_gpgpu_VERSION} CMAKE_ARGS ${ogles_gpgpu_cmake_args})

### ogles_gpgpu ###
set(ogles_gpgpu_cmake_args
  OGLES_GPGPU_VERBOSE=OFF
  OGLES_GPGPU_OPENGL_ES3=${ACF_OPENGL_ES3}
)
hunter_config(ogles_gpgpu VERSION ${HUNTER_ogles_gpgpu_VERSION} CMAKE_ARGS ${ogles_gpgpu_cmake_args})

### opencv ###
set(opencv_cmake_args
  BUILD_LIST=core,imgproc,videoio,highgui
)
hunter_config(OpenCV VERSION ${HUNTER_OpenCV_VERSION} CMAKE_ARGS ${opencv_cmake_args})

### ACF (GIT_SELF) : support relocatable package testing ###
set(acf_cmake_args
  ACF_BUILD_TESTS=${ACF_BUILD_TESTS}
  ACF_BUILD_EXAMPLES=${ACF_BUILD_EXAMPLES}
  ACF_SERIALIZE_WITH_CVMATIO=${ACF_SERIALIZE_WITH_CVMATIO}
  ACF_SERIALIZE_WITH_CEREAL=ON
  ACF_BUILD_OGLES_GPGPU=${ACF_BUILD_OGLES_GPGPU}
)
hunter_config(acf GIT_SELF CMAKE_ARGS ${acf_cmake_args})


