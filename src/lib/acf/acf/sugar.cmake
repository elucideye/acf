# This file generated automatically by:
#   generate_sugar_files.py
# see wiki for more info:
#   https://github.com/ruslo/sugar/wiki/Collecting-sources

if(DEFINED LIB_ACF_ACF_SUGAR_CMAKE_)
  return()
else()
  set(LIB_ACF_ACF_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_SRCS
  ACF.cpp
  ACFIO.cpp # optional
  ACFIOArchiveCereal.cpp
  MatP.cpp
  ObjectDetector.cpp
  acfModify.cpp
  bbNms.cpp
  chnsCompute.cpp
  chnsPyramid.cpp
  convTri.cpp
  draw.cpp
  gradientHist.cpp
  gradientMag.cpp
  rgbConvert.cpp
  transfer.cpp
  #######################
  ### Toolbox sources ###
  #######################  
  toolbox/acfDetect1.cpp
  toolbox/convConst.cpp
  toolbox/gradientMex.cpp
  toolbox/imPadMex.cpp
  toolbox/imResampleMex.cpp
  toolbox/rgbConvertMex.cpp
  toolbox/wrappers.cpp
)

sugar_files(ACF_HDRS
  ACFIO.h
  ACFIOArchive.h
  ACFObject.h
  ObjectDetector.h
  random.h
  transfer.h
  #######################
  ### Toolbox headers ###
  #######################  
  toolbox/sse.hpp
  toolbox/wrappers.hpp
  )

sugar_files(ACF_HDRS_PUBLIC
  ACF.h
  ACFField.h
  ObjectDetector.h
  MatP.h
  acf_common.h
  draw.h
)

# Collect sources for testing
sugar_files(ACF_TEST_SRCS
  convert.cpp
  convert.h
)

if(ACF_BUILD_OGLES_GPGPU)
  # Public GPUACF classes added to the main library
  sugar_files(ACF_HDRS_PUBLIC GPUACF.h)
  sugar_files(ACF_HDRS convert.h) 
  sugar_files(ACF_SRCS GPUACF.cpp convert.cpp)

  # All other shaders to in the acf_shaders utility lib for reuse
  sugar_files(ACF_GPU_HDRS
    gpu/gradhist.h
    gpu/multipass/triangle_pass.h
    gpu/multipass/triangle_opt_pass.h
    gpu/swizzle2.h
    gpu/triangle.h
    gpu/triangle_opt.h    
    gpu/binomial.h
  )
  sugar_files(ACF_GPU_SRCS
    gpu/gradhist.cpp
    gpu/multipass/triangle_pass.cpp
    gpu/multipass/triangle_opt_pass.cpp
    gpu/swizzle2.cpp
    gpu/triangle.cpp
    gpu/triangle_opt.cpp
    gpu/binomial.cpp
  )

  # Collect private unit test sources
  sugar_Files(ACF_TEST_SRCS
    transfer.cpp
    transfer.h    
    gpu/triangle_opt.h
    gpu/triangle_opt.cpp
    gpu/multipass/triangle_opt_pass.h
    gpu/multipass/triangle_opt_pass.cpp
  )
  
endif()
