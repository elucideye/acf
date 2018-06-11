if(DEFINED LIB_ACF_IO_SUGAR_CMAKE_)
  return()
else()
  set(LIB_ACF_IO_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_HDRS
  cereal_pba.h
  cvmat_cereal.h
)
