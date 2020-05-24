cmake_minimum_required(VERSION 3.14)

project(Metabox LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE ON)

if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1 /fp:strict")
    set(CMAKE_CXX_FLAGS_MINSIZEREL     "/MD /O1 /Ob1")
    set(CMAKE_CXX_FLAGS_RELEASE        "/MD /O2 /Ob2")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MD /Zi /O2 /Ob1")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_HAS_AUTO_PTR_ETC=1)
endif()

# Build GLEW as a static lib
#add_definitions(-DGLEW_STATIC)
#option( glew-cmake_BUILD_SHARED "Build the shared glew library" OFF )
#option( glew-cmake_BUILD_STATIC "Build the static glew library" ON )

# Build SFML as a static lib
option( BUILD_SHARED_LIBS "Build the shared libraries" OFF )
option( SFML_USE_STATIC_STD_LIBS "Build the static sfml library" TRUE )
set(SFML_STATIC_LIBRARIES TRUE)
add_compile_definitions(SFML_STATIC)

# Build Box2D as a static lib
option(BOX2D_BUILD_SHARED "Build Box2D shared libraries" OFF)
option(BOX2D_BUILD_STATIC "Build Box2D static libraries" ON)
option(BOX2D_BUILD_EXAMPLES "Build Box2D examples" OFF)

# Build dependencies
add_subdirectory(sfml)
add_subdirectory(box2d/Box2D)
#add_subdirectory(glew)
#add_subdirectory(glfw)

# Collect sources for in-built dependencies
#file(GLOB imgui_sources imgui/*.cpp)
file(GLOB_RECURSE metabox_sources metabox/*.cpp)

add_executable(metabox ${metabox_sources})

target_compile_features(metabox PUBLIC cxx_std_17)
target_compile_definitions(metabox PRIVATE -D_SCL_SECURE_NO_WARNINGS)

target_include_directories(metabox PUBLIC metabox)
target_include_directories(metabox PUBLIC box2d/Box2D)
target_include_directories(metabox PUBLIC sfml/include)

# Link dependencies
#target_link_libraries(metabox libglew_static ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
#target_link_libraries(metabox Box2D ${SFML_LIBRARIES} ${SFML_DEPENDENCIES})
#target_link_libraries(metabox Box2D sfml-system-s-d sfml-audio-s-d sfml-window-s-d sfml-graphics-s-d sfml-main-s-d sfml-network-s-d)
target_link_libraries(metabox Box2D sfml-system sfml-audio sfml-window sfml-graphics sfml-main sfml-network)

message(STATUS "metabox_sources: ${metabox_sources}")
message(STATUS "SFML_LIBRARIES: ${SFML_LIBRARIES}")
message(STATUS "SFML_DEPENDENCIES: ${SFML_DEPENDENCIES}")

# Copy assets
file(GLOB_RECURSE asset_files assets/*.*)
message(STATUS "assets: ${asset_files}")
foreach(file ${asset_files})
    get_filename_component(file_name ${file} NAME)
    configure_file(${file} ${CMAKE_CURRENT_BINARY_DIR}/${file_name} COPYONLY)
endforeach()
