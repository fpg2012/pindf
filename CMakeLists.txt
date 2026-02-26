cmake_minimum_required(VERSION 3.10)
project(pindf LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug or Release)" FORCE)
endif()

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PINDF_OS "Linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PINDF_OS "macOS")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PINDF_OS "Windows")
endif()

# Find zlib
find_package(ZLIB REQUIRED)

# Library source files
set(LIB_SRC_FILES
    pdf/doc.c
    pdf/modif.c
    pdf/obj.c
    pdf/xref.c
    container/simple_vector.c
    container/uchar_str.c
    core/lexer.c
    core/parser.c
    core/serialize.c
    stream/filter.c
    pindf.c
    logger/logger.c
)

# Create shared library
add_library(pindf SHARED ${LIB_SRC_FILES})

# Create static library
add_library(pindf_static STATIC ${LIB_SRC_FILES})
set_target_properties(pindf_static PROPERTIES OUTPUT_NAME "pindf")

# Include directories for static library
target_include_directories(pindf_static PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/container
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/pdf
    ${CMAKE_CURRENT_SOURCE_DIR}/stream
    ${CMAKE_CURRENT_SOURCE_DIR}/logger
)

# Link zlib for static library
target_link_libraries(pindf_static PRIVATE ZLIB::ZLIB)

# Set library properties based on platform
if(PINDF_OS STREQUAL "Linux")
    set_target_properties(pindf PROPERTIES
        OUTPUT_NAME "pindf"
        PREFIX ""
        SUFFIX ".so"
    )
elseif(PINDF_OS STREQUAL "macOS")
    set_target_properties(pindf PROPERTIES
        OUTPUT_NAME "pindf"
        PREFIX ""
        SUFFIX ".dylib"
        INSTALL_NAME_DIR "@rpath"
        BUILD_WITH_INSTALL_RPATH ON
    )
elseif(PINDF_OS STREQUAL "Windows")
    set_target_properties(pindf PROPERTIES
        OUTPUT_NAME "pindf"
        PREFIX ""
        SUFFIX ".dll"
    )
endif()

# Include directories
target_include_directories(pindf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/container
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/pdf
    ${CMAKE_CURRENT_SOURCE_DIR}/stream
    ${CMAKE_CURRENT_SOURCE_DIR}/logger
)

# Link zlib
target_link_libraries(pindf PRIVATE ZLIB::ZLIB)

# Optional address sanitizer (like Makefile line 37)
option(PINDF_USE_ASAN "Enable address sanitizer" OFF)
if(PINDF_USE_ASAN)
    target_compile_options(pindf PRIVATE -fsanitize=address)
    target_link_options(pindf PRIVATE -fsanitize=address)
    target_compile_options(pindf_static PRIVATE -fsanitize=address)
    target_link_options(pindf_static PRIVATE -fsanitize=address)
endif()

# Compiler flags based on build type
target_compile_options(pindf PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>:-O2 -Wall -DNDEBUG>
)

target_compile_options(pindf_static PRIVATE
    $<$<CONFIG:Debug>:-g -Wall>
    $<$<CONFIG:Release>:-O2 -Wall -DNDEBUG>
)

# Position independent code for shared library
set_target_properties(pindf PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Test executables
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test")
    # Test source files list
    set(TEST_SOURCES
        test/lexer_test.c
        test/parser_test.c
        test/parser_from_buffer_test.c
        test/vec_test.c
        test/modif_test.c
        test/compress_test.c
        test/dict_test.c
        test/doc_save_modif_test.c
    )

    # Check each test file exists
    foreach(test_source ${TEST_SOURCES})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")
            get_filename_component(test_name ${test_source} NAME_WE)

            # Special handling for vec_test (only uses simple_vector.c)
            if(test_name STREQUAL "vec_test")
                add_executable(${test_name} ${test_source} container/simple_vector.c)
                target_include_directories(${test_name} PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/container
                )
            else()
                # Other tests use all library sources
                add_executable(${test_name} ${test_source} ${LIB_SRC_FILES})
                target_include_directories(${test_name} PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/container
                    ${CMAKE_CURRENT_SOURCE_DIR}/core
                    ${CMAKE_CURRENT_SOURCE_DIR}/pdf
                    ${CMAKE_CURRENT_SOURCE_DIR}/stream
                    ${CMAKE_CURRENT_SOURCE_DIR}/logger
                )
                target_link_libraries(${test_name} PRIVATE ZLIB::ZLIB)
            endif()

            # Set output name
            set_target_properties(${test_name} PROPERTIES OUTPUT_NAME "${test_name}")

            # Add to tests list
            list(APPEND TEST_TARGETS ${test_name})
        endif()
    endforeach()

    # Create custom target to build all tests (matches Makefile's 'test' target)
    if(TEST_TARGETS)
        add_custom_target(build_tests DEPENDS ${TEST_TARGETS})
        add_custom_target(test DEPENDS build_tests)
    endif()
endif()

# Documentation target (matches Makefile)
add_custom_target(doc
    COMMAND mkdir -p docs/doxygen
    COMMAND doxygen Doxyfile
    COMMAND cd docs && sphinx-build -b html . _build/html
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating documentation"
)

# Clean documentation target (matches Makefile)
add_custom_target(clean_doc
    COMMAND rm -rf docs/_build
    COMMAND rm -rf docs/doxygen
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Cleaning documentation"
)

# Dylib target (alias, matches Makefile)
add_custom_target(dylib DEPENDS pindf)

# Install targets
install(TARGETS pindf
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)