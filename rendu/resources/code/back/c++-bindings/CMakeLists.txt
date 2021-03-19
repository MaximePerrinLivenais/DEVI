find_package(pybind11 CONFIG REQUIRED)


pybind11_add_module(soducocxx MODULE
  src/Application-wrapper.cpp
  src/ndimage_buffer_helper.cpp
  src/DOMTypes-wrapper.cpp
  )

target_link_libraries(soducocxx PRIVATE Pylene::Pylene soduco)


