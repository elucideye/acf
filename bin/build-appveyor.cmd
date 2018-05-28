:: Name: build-appveyor.cmd
:: Purpose: Support readable multi-line polly.py build commands
:: Copyright 2016-2018 Elucideye, Inc.
::
:: Multi-line commands are not currently supported directly in appveyor.yml files
::
:: See: http://stackoverflow.com/a/37647169

echo POLLY_ROOT %POLLY_ROOT%

python %POLLY_ROOT%\bin\polly.py ^
--verbose ^
--archive acf ^
--config "%1%" ^
--toolchain "%2%" ^
--test ^
--fwd ^
ACF_USE_DRISHTI_UPLOAD=YES ^
ACF_BUILD_SHARED_SDK="%3" ^
ACF_COPY_3RDPARTY_LICENSES=ON ^
ACF_BUILD_TESTS=ON ^
ACF_BUILD_EXAMPLES=ON %APPVEYOR_CMAKE_ARGS%
