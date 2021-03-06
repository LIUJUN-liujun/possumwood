# Find the source files
file(GLOB_RECURSE headers *.h)
file(GLOB_RECURSE sources *.cpp)
file(GLOB sources_common ../*.cpp)

# Find OpenImageIO using a custom module
include(possumwood_oiio)

# Find OpenGL
#FIND_PACKAGE(OpenGL REQUIRED)
#set(LIBS ${LIBS} ${OPENGL_LIBRARIES})

# Find GLUT
#FIND_PACKAGE(GLUT REQUIRED)
#message(info ${GLUT_INCLUDE_DIRS})
#set(LIBS ${LIBS} ${GLUT_LIBRARY})

# Find opencv - cannot be dynamically loaded due to use of jemalloc
find_package(OpenCV REQUIRED)
set(LIBS ${LIBS} ${OpenCV_LIBS})

# Build the executable
include_directories(./)
add_executable(possumwood_cli ${sources} ${sources_common})
install(TARGETS possumwood_cli DESTINATION bin)

target_link_libraries(possumwood_cli -Wl,--no-as-needed dependency_graph possumwood_sdk ${LIBS} dl GLEW GLU GL glfw)
