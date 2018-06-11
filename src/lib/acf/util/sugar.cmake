if(DEFINED LIB_ACF_UTIL_SUGAR_CMAKE_)
  return()
else()
  set(LIB_ACF_UTIL_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_UTIL_HDRS
  IndentingOStreamBuffer.h
  acf_math.h
  acf_util.h
  make_unique.h
  ordered.h
  string_hash.h
)
