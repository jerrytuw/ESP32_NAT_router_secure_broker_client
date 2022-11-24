# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/jerry/esp/esp-idf/components/bootloader/subproject"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix/tmp"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix/src"
  "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/jerry/Documents/Platformio/esp32_nat_router2/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
