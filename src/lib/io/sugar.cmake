if(DEFINED LIB_IO_SUGAR_CMAKE_)
  return()
else()
  set(LIB_IO_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_HDRS
  cereal_pba.h
  cv_cereal.h
  cvmat_cereal.h
  )

if(ACF_ADD_TO_STRING)
  sugar_files(ACF_HDRS stdlib_string.h)
endif()