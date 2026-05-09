if("${IDF_TARGET}" STREQUAL "linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED sdl3)
    idf_component_register(
        SRC_DIRS "src/common" "src/client" "src/lib" # "src/server"
        INCLUDE_DIRS "include"
        REQUIRES engine)
    target_include_directories(${COMPONENT_LIB} PRIVATE ${SDL3_INCLUDE_DIRS})
    target_compile_options(${COMPONENT_LIB} PRIVATE ${SDL3_CFLAGS_OTHER})
    target_link_libraries(${COMPONENT_LIB} PRIVATE ${SDL3_LIBRARIES} m)
else()
    idf_component_register(
        SRC_DIRS "src/common" "src/client" "src/lib" # "src/server"
        INCLUDE_DIRS "include"
        REQUIRES engine ili9486-8bit-parallel lwip
    )
endif()
