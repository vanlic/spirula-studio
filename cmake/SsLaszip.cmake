include(FetchContent)
set(LASZIP_BUILD_STATIC ON CACHE INTERNAL "Link the point cloud decoder into Spirula")
FetchContent_Declare(laszip
    URL https://codeload.github.com/LASzip/LASzip/tar.gz/refs/tags/3.5.0
    URL_HASH SHA256=6e9baac8689dfd2e1502ceafabb20c62b6cd572744d240fb755503fd57c2a6af
    SOURCE_SUBDIR cmake)
FetchContent_MakeAvailable(laszip)
add_subdirectory("${laszip_SOURCE_DIR}" "${laszip_BINARY_DIR}" EXCLUDE_FROM_ALL)

set(SS_LASZIP_LIB laszip)
if(WIN32)
    set(SS_LASZIP_LIB laszip3)
endif()
set_property(TARGET ${SS_LASZIP_LIB} PROPERTY CXX_STANDARD 17)
if(MSVC)
    target_compile_options(${SS_LASZIP_LIB} PRIVATE /EHsc)
endif()
configure_file("${laszip_SOURCE_DIR}/COPYING.txt" "${CMAKE_BINARY_DIR}/LASzip-LICENSE.txt" COPYONLY)
add_library(ss_laszip INTERFACE)
target_include_directories(ss_laszip SYSTEM INTERFACE
    "${laszip_SOURCE_DIR}/dll" "${laszip_SOURCE_DIR}/include/laszip")
target_link_libraries(ss_laszip INTERFACE ${SS_LASZIP_LIB})
