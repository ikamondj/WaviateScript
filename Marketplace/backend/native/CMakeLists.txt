cmake_minimum_required(VERSION 3.24)

project(WaviateMarketplaceNativeCompiler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(WAVIATESCRIPT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." CACHE PATH "Root of the WaviateScript desktop source tree")
set(WAVIATESCRIPT_JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout. If empty, CMake fetches JUCE.")
set(WAVIATESCRIPT_JUCE_TAG "8.0.12" CACHE STRING "Pinned JUCE tag or commit used when WaviateScript fetches JUCE")

if(NOT EXISTS "${WAVIATESCRIPT_ROOT}/Source/CompilePipeline.cpp")
    message(FATAL_ERROR "WAVIATESCRIPT_ROOT does not point at the WaviateScript source tree: ${WAVIATESCRIPT_ROOT}")
endif()

if(NOT WAVIATESCRIPT_JUCE_DIR AND EXISTS "${WAVIATESCRIPT_ROOT}/_deps/juce-src/CMakeLists.txt")
    set(WAVIATESCRIPT_JUCE_DIR "${WAVIATESCRIPT_ROOT}/_deps/juce-src" CACHE PATH "Path to a local JUCE checkout. If empty, CMake fetches JUCE." FORCE)
endif()

include(FetchContent)

if(WAVIATESCRIPT_JUCE_DIR)
    if(NOT EXISTS "${WAVIATESCRIPT_JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "WAVIATESCRIPT_JUCE_DIR does not point to a JUCE checkout with CMakeLists.txt: ${WAVIATESCRIPT_JUCE_DIR}")
    endif()
    add_subdirectory("${WAVIATESCRIPT_JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG "${WAVIATESCRIPT_JUCE_TAG}"
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

function(waviate_first_existing_path output_variable)
    foreach(candidate IN LISTS ARGN)
        if(candidate AND EXISTS "${candidate}")
            set("${output_variable}" "${candidate}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

if(WIN32)
    if(NOT LLVM_DIR)
        waviate_first_existing_path(WAVIATESCRIPT_DETECTED_LLVM_DIR
            "$ENV{LLVM_DIR}"
            "$ENV{ProgramFiles}/LLVM/lib/cmake/llvm"
            "$ENV{ProgramW6432}/LLVM/lib/cmake/llvm"
        )
        if(WAVIATESCRIPT_DETECTED_LLVM_DIR)
            set(LLVM_DIR "${WAVIATESCRIPT_DETECTED_LLVM_DIR}" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
        endif()
    endif()

    if(NOT Clang_DIR)
        waviate_first_existing_path(WAVIATESCRIPT_DETECTED_CLANG_DIR
            "$ENV{Clang_DIR}"
            "$ENV{ProgramFiles}/LLVM/lib/cmake/clang"
            "$ENV{ProgramW6432}/LLVM/lib/cmake/clang"
        )
        if(WAVIATESCRIPT_DETECTED_CLANG_DIR)
            set(Clang_DIR "${WAVIATESCRIPT_DETECTED_CLANG_DIR}" CACHE PATH "Path to ClangConfig.cmake" FORCE)
        endif()
    endif()
endif()

find_package(LLVM CONFIG REQUIRED)
find_package(Clang CONFIG REQUIRED)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
message(STATUS "Using Clang package from ${Clang_DIR}")

function(waviate_configure_msvc_llvm_compat target_name)
    if(MSVC)
        set_property(TARGET "${target_name}" PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
        target_compile_definitions("${target_name}" PRIVATE $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)
    endif()
endfunction()

if(WIN32 AND TARGET LLVMDebugInfoPDB)
    set(WAVIATESCRIPT_DIAGUIDS_LIB "")
    if(CMAKE_GENERATOR_INSTANCE AND EXISTS "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64/diaguids.lib")
        set(WAVIATESCRIPT_DIAGUIDS_LIB "${CMAKE_GENERATOR_INSTANCE}/DIA SDK/lib/amd64/diaguids.lib")
    elseif(EXISTS "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
        set(WAVIATESCRIPT_DIAGUIDS_LIB "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Community/DIA SDK/lib/amd64/diaguids.lib")
    endif()

    if(WAVIATESCRIPT_DIAGUIDS_LIB)
        get_target_property(WAVIATESCRIPT_LLVM_PDB_LINK_LIBS LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
        if(WAVIATESCRIPT_LLVM_PDB_LINK_LIBS)
            string(REPLACE
                "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib"
                "${WAVIATESCRIPT_DIAGUIDS_LIB}"
                WAVIATESCRIPT_LLVM_PDB_LINK_LIBS
                "${WAVIATESCRIPT_LLVM_PDB_LINK_LIBS}"
            )
            set_target_properties(LLVMDebugInfoPDB PROPERTIES
                INTERFACE_LINK_LIBRARIES "${WAVIATESCRIPT_LLVM_PDB_LINK_LIBS}"
            )
            message(STATUS "Adjusted LLVMDebugInfoPDB to use ${WAVIATESCRIPT_DIAGUIDS_LIB}")
        endif()
    else()
        message(WARNING "Could not locate a DIA SDK diaguids.lib to override LLVM's stale imported path. LLVMDebugInfoPDB may fail to link.")
    endif()
endif()

set(WAVIATESCRIPT_COMPILER_SOURCES
    "${WAVIATESCRIPT_ROOT}/Source/AbstractCompiler.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/CompilePipeline.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/WaviateAudio.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/WaviateCppLanguageModel.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/WaviateFuelInstrumentation.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/WaviateSafety.cpp"
    "${WAVIATESCRIPT_ROOT}/Source/WaviateSafetyValidator.cpp"
)

set(WAVIATESCRIPT_CLANG_TARGETS
    clangFrontend
    clangDriver
    clangParse
    clangSema
    clangSerialization
    clangAST
    clangLex
    clangBasic
    clangCodeGen
    clangTooling
    clangToolingCore
    clangAnalysis
    clangEdit
    clangASTMatchers
    clangDynamicASTMatchers
)

function(waviate_configure_compiler_target target_name)
    target_include_directories("${target_name}"
        PUBLIC
            "${CMAKE_CURRENT_LIST_DIR}/include"
            "${WAVIATESCRIPT_ROOT}/Source"
            "${LLVM_INCLUDE_DIRS}"
            "${CLANG_INCLUDE_DIRS}"
    )

    target_link_libraries("${target_name}"
        PUBLIC
            juce::juce_core
    )

    foreach(clang_target IN LISTS WAVIATESCRIPT_CLANG_TARGETS)
        if(TARGET "${clang_target}")
            target_link_libraries("${target_name}" PUBLIC "${clang_target}")
        endif()
    endforeach()

    if(COMMAND llvm_map_components_to_libnames)
        llvm_map_components_to_libnames(WAVIATESCRIPT_LLVM_LIBS
            native
            nativecodegen
            orcjit
            mcjit
            executionengine
            core
            support
            target
            analysis
            passes
            scalaropts
            instcombine
            bitreader
            bitwriter
            irreader
            object
        )
        target_link_libraries("${target_name}" PUBLIC ${WAVIATESCRIPT_LLVM_LIBS})
    endif()

    waviate_configure_msvc_llvm_compat("${target_name}")
endfunction()

add_library(waviate_marketplace_compiler STATIC
    "src/waviate_marketplace_compiler.cpp"
    ${WAVIATESCRIPT_COMPILER_SOURCES}
)
waviate_configure_compiler_target(waviate_marketplace_compiler)

if(WIN32)
    add_library(waviate_marketplace_compiler_runtime SHARED
        "src/waviate_marketplace_compiler.cpp"
        ${WAVIATESCRIPT_COMPILER_SOURCES}
    )
    set_target_properties(waviate_marketplace_compiler_runtime PROPERTIES
        OUTPUT_NAME "waviate_marketplace_compiler_runtime"
    )
    target_compile_definitions(waviate_marketplace_compiler_runtime
        PRIVATE
            WAVIATESCRIPT_MARKETPLACE_COMPILER_DLL=1
            WAVIATESCRIPT_MARKETPLACE_COMPILER_BUILDING_DLL=1
    )
    waviate_configure_compiler_target(waviate_marketplace_compiler_runtime)
endif()
