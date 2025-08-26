# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/pico/build-playground/_deps/picotool-src/enc_bootloader")
  file(MAKE_DIRECTORY "C:/pico/build-playground/_deps/picotool-src/enc_bootloader")
endif()
file(MAKE_DIRECTORY
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls"
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls"
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/tmp"
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/src/enc_bootloader_mbedtls-stamp"
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/src"
  "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/src/enc_bootloader_mbedtls-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/src/enc_bootloader_mbedtls-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/pico/build-playground/_deps/picotool-build/enc_bootloader_mbedtls/src/enc_bootloader_mbedtls-stamp${cfgdir}") # cfgdir has leading slash
endif()
