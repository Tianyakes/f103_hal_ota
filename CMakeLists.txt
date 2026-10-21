cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME ota)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
    Bootloader/core/bl_conf.h
    Bootloader/core/bl_version.h
    Bootloader/core/bootloader.c
    Bootloader/core/bootloader.h
    Bootloader/core/boot_manager.c
    Bootloader/core/boot_manager.h
    Bootloader/Driver/uart_drv.c
    Bootloader/Driver/uart_drv.h
    Bootloader/Driver/flash_drv.c
    Bootloader/Driver/flash_drv.h
    Bootloader/Driver/wdg_drv.c
    Bootloader/Driver/wdg_drv.h
    Bootloader/Driver/jump_to_app.c
    Bootloader/Driver/jump_to_app.h
    Bootloader/Protocol/ota_protocol.c
    Bootloader/Protocol/ota_protocol.h
    Bootloader/Protocol/uart_transport.c
    Bootloader/Protocol/uart_transport.h
    Bootloader/service/crc16.c
    Bootloader/service/crc16.h
    Bootloader/service/flash_if.c
    Bootloader/service/flash_if.h
    Bootloader/service/flag_manager.c
    Bootloader/service/flag_manager.h
    Bootloader/service/app_check.c
    Bootloader/service/app_check.h
    Bootloader/log_src/bl_log.c
    Bootloader/log_src/bl_log.h)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Bootloader/core
    Bootloader/Driver
    Bootloader/Protocol
    Bootloader/service
    Bootloader/log_src
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
