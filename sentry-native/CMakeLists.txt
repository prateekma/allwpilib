# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in sentry-native-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sentry-native-download )
if(result)
  message(FATAL_ERROR "CMake step for sentry-native failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/sentry-native-download )
if(result)
  message(FATAL_ERROR "Build step for sentry-native failed: ${result}")
endif()

set(SENTRY_BACKEND "crashpad" CACHE STRING "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/sentry-native-src
                 ${CMAKE_CURRENT_BINARY_DIR}/sentry-native-build
                 EXCLUDE_FROM_ALL)
