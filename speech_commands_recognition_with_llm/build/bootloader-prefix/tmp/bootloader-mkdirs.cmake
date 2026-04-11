# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/ESP32-IDF/Espressif/frameworks/esp-idf-v5.3.1/components/bootloader/subproject"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/tmp"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/src"
  "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Project/GitHub/xiaozhi-replica-main/examples/speech_commands_recognition_with_llm/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
