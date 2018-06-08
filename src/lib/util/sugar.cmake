if(DEFINED LIB_UTIL_SUGAR_CMAKE_)
  return()
else()
  set(LIB_UTIL_SUGAR_CMAKE_ 1)
endif()

include(sugar_files)

sugar_files(ACF_UTIL_HDRS
  IndentingOStreamBuffer.h
  LazyParallelResource.h
  Line.h
  Logger.h
  acf_math.h
  acf_util.h
  cli.h
  make_unique.h
  ordered.h
  string_hash.h
  string_utils.h
  timing.h
)

sugar_files(ACF_UTIL_SRCS
  Logger.cpp
  string_utils.cpp
)
