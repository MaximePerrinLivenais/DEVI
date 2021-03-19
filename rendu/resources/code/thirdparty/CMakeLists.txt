include(FetchContent)


FetchContent_Declare(
  blend2d
  GIT_REPOSITORY https://github.com/blend2d/blend2d
  )

FetchContent_Declare(
  asmjit
  GIT_REPOSITORY https://github.com/asmjit/asmjit
  GIT_TAG master
  )


FetchContent_GetProperties(asmjit)
if(NOT asmjit_POPULATED)
  FetchContent_Populate(asmjit)
endif()

FetchContent_GetProperties(blend2d)
if(NOT blend2d_POPULATED)
  FetchContent_Populate(blend2d)
  set(ASMJIT_DIR ${asmjit_SOURCE_DIR} CACHE PATH "Location of 'asmjit'")
  add_subdirectory(${blend2d_SOURCE_DIR} ${blend2d_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_subdirectory(LSD)
#add_subdirectory(pylene)
