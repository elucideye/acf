###################
## Dependencies ###
###################

# OpenCV
hunter_add_package(OpenCV)
find_package(OpenCV REQUIRED)
list(APPEND ACF_3RDPARTY_PKG_LIBS ${OpenCV_LIBS})

# ogles_gpgpu
if(ACF_BUILD_OGLES_GPGPU)
  hunter_add_package(ogles_gpgpu)
  find_package(ogles_gpgpu CONFIG REQUIRED)
  list(APPEND ACF_3RDPARTY_PKG_LIBS ogles_gpgpu::ogles_gpgpu)
endif()

# spdlog
hunter_add_package(spdlog)
find_package(spdlog REQUIRED)
list(APPEND ACF_3RDPARTY_PKG_LIBS spdlog::spdlog)

# cereal
if(ACF_SERIALIZE_WITH_CEREAL)
  hunter_add_package(cereal)
  find_package(cereal CONFIG REQUIRED)
  list(APPEND ACF_3RDPARTY_PKG_LIBS cereal::cereal)
endif()

# half
hunter_add_package(half)
find_package(half CONFIG REQUIRED)
list(APPEND ACF_3RDPARTY_PKG_LIBS half::half)

# cvmatio
if(ACF_SERIALIZE_WITH_CVMATIO)
  hunter_add_package(cvmatio)
  find_package(cvmatio REQUIRED CONFIG)
  list(APPEND ACF_3RDPARTY_PKG_LIBS cvmatio::cvmatio)
endif()

# NEON_2_SSE ### (header only)
hunter_add_package(ARM_NEON_2_x86_SSE)
find_package(ARM_NEON_2_x86_SSE CONFIG REQUIRED)
list(APPEND ACF_SDK_3RDPARTY_PKG_LIBS ARM_NEON_2_x86_SSE::ARM_NEON_2_x86_SSE)

# sse2neon ### (header only)
hunter_add_package(sse2neon)
find_package(sse2neon CONFIG REQUIRED)
list(APPEND ACF_SDK_3RDPARTY_PKG_LIBS sse2neon::sse2neon) # Add library

hunter_add_package(Sugar) 

add_subdirectory(lib)

##################
## Applications ##
##################

add_subdirectory(app)

