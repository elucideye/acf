# This file generated automatically by:
#   generate_sugar_files.py
# see wiki for more info:
#   https://github.com/ruslo/sugar/wiki/Collecting-sources

if(DEFINED LIB_ACF_SUGAR_CMAKE_)
  return()
else()
  set(LIB_ACF_SUGAR_CMAKE_ 1)
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
  draw.h
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
  )

if(ACF_BUILD_OGLES_GPGPU)
  sugar_files(ACF_HDRS_PUBLIC 
    GPUACF.h
    )
  sugar_files(ACF_HDRS
    gpu/gradhist.h
    gpu/multipass/triangle_pass.h
    gpu/swizzle2.h
    gpu/triangle.h
    gpu/binomial.h
    )
  sugar_files(ACF_SRCS
    GPUACF.cpp
    gpu/gradhist.cpp
    gpu/multipass/triangle_pass.cpp
    gpu/swizzle2.cpp
    gpu/triangle.cpp
    gpu/binomial.cpp
    )
endif()
