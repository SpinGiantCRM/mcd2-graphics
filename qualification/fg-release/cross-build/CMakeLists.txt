cmake_minimum_required(VERSION 3.23)
project(MatchedReShadeBuild LANGUAGES C CXX RC)
if(NOT DEFINED FRAMEWORK_SOURCE)
  message(FATAL_ERROR "A pinned framework source directory is required")
endif()
add_subdirectory("${FRAMEWORK_SOURCE}" framework)
# Shared Clang-cl orchestration; no framework source edits.
target_compile_options(fpng PRIVATE /clang:-mpclmul /clang:-msse4.1)
target_link_libraries(ReShade PRIVATE delayimp.lib)
target_link_options(ReShade PRIVATE /DELAYLOAD:D3DCompiler_47.dll /DELAYLOAD:WinMM.dll /DELAYLOAD:XInput9_1_0.dll)

# POSIX llvm-rc does not normalize Windows resource filenames. Preserve bytes
# and resource IDs, changing only host filenames in a generated RC copy.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/normalize_resources.py" "${FRAMEWORK_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/resource-posix.rc" COMMAND_ERROR_IS_FATAL ANY)
get_target_property(framework_sources ReShade SOURCES)
list(FILTER framework_sources EXCLUDE REGEX "res/resource\\.rc$")
set_property(TARGET ReShade PROPERTY SOURCES "${framework_sources}")
target_sources(ReShade PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/resource-posix.rc")

target_compile_options(ReShade PRIVATE /clang:-mf16c /clang:-msse4.1)
target_compile_options(ReShade PRIVATE /clang:-Wno-c++11-narrowing)

# Resolve Clang's block-scope linkage spelling to the upstream global symbols.
# These aliases share the same objects; no replacement implementations.
target_link_options(ReShade PRIVATE
 "/alternatename:get_system_path=?get_system_path@@YA?AVpath@filesystem@std@@XZ"
 "/alternatename:?g_opengl_context@openvr@reshade@@3PEAVdevice_context_impl@opengl@2@EA=?g_opengl_context@@3PEAVdevice_context_impl@opengl@reshade@@EA"
 "/alternatename:?g_vulkan_devices@openvr@reshade@@3V?$lockfree_linear_map@PEAXPEAVdevice_impl@vulkan@reshade@@$07@@A=?g_vulkan_devices@@3V?$lockfree_linear_map@PEAXPEAVdevice_impl@vulkan@reshade@@$07@@A")
target_compile_options(MinHook PRIVATE /FIintrin.h)
