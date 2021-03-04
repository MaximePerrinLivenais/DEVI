project(soduco)
cmake_minimum_required(VERSION 3.11)


include("${CMAKE_BINARY_DIR}/conan_paths.cmake")


find_package(PkgConfig REQUIRED)
find_package(CLI11 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt 6.0 REQUIRED)
find_package(Pylene REQUIRED)
find_package(FreeImage REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

pkg_check_modules(poppler-cpp REQUIRED IMPORTED_TARGET poppler-cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options("-Wno-deprecated-declarations")
add_subdirectory(thirdparty)



add_compile_options("-W" "-Wall")
add_library(soduco
  sources/src/Application.cpp
  sources/src/DOMTypes.cpp
  sources/src/CoreTypes.cpp
  sources/src/PDFInfo.cpp

  sources/src/load_pages.hpp
  sources/src/load_pages.cpp
  sources/src/detect_separators.hpp
  sources/src/detect_separators.cpp
  sources/src/deskew.hpp
  sources/src/deskew.cpp
  sources/src/subsample.hpp
  sources/src/subsample.cpp
  sources/src/config.hpp
  sources/src/config.cpp
  sources/src/gaussian_directional_2d.hpp
  sources/src/gaussian_directional_2d.cpp


  sources/src/Interval.hpp
  sources/src/Interval.cpp

  sources/src/DOMBuilder_helpers.hpp
  sources/src/DOMBuilder_helpers.cpp
  sources/src/DOMBlocksExtractor.hpp
  sources/src/DOMBlocksExtractor.cpp
  sources/src/DOMLinesExtractor.hpp
  sources/src/DOMLinesExtractor.cpp
  sources/src/DOMEntriesExtractor.hpp
  sources/src/DOMEntriesExtractor.cpp
  sources/src/DOMTextExtractor.hpp
  sources/src/DOMTextTesseractExtractor.cpp
  #sources/src/DOMTextExtractor.cpp
  )

target_include_directories(soduco PUBLIC sources/include)
target_link_libraries(soduco PRIVATE PkgConfig::poppler-cpp LSD spdlog::spdlog tesseract)
target_link_libraries(soduco PUBLIC Pylene::Pylene)

add_executable(soduco-cli
  sources/src/cli.cpp

  sources/src/DOMExport.hpp
  sources/src/DOMExport.cpp
  sources/src/display.hpp
  sources/src/display.cpp
  sources/src/region_lut.hpp
  sources/src/region_lut.cpp

  )
target_link_libraries(soduco-cli PRIVATE CLI11::CLI11 spdlog::spdlog nlohmann_json::nlohmann_json blend2d soduco)
target_compile_features(soduco-cli PUBLIC cxx_std_20)


add_subdirectory(back/c++-bindings)




include(GNUInstallDirs)
install(TARGETS soduco-cli RUNTIME DESTINATION bin)
install(TARGETS blend2d LIBRARY DESTINATION lib)
install(TARGETS soducocxx LIBRARY DESTINATION "./back")
set(CPACK_GENERATOR "ZIP")
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}-py${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
include(CPack)
