
include_directories(include)
include_directories(${ASSIMP_INC})
include_directories(${GLM_INC})
include_directories(${GLEW_INC})
include_directories(${OPENGL_INC})
include_directories(${GLUT_INC})

file(GLOB_RECURSE SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS src/*.cpp)
file(GLOB_RECURSE INC RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS include/*.h)
file(GLOB_RECURSE RESOURCE_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} FOLLOW_SYMLINKS resources/*)

foreach(RF ${RESOURCE_FILES})
	file(COPY ${RF} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endforeach(RF ${RESOURCE_FILES})

add_executable(Week11-CodeProvided ${SRCS} ${INC})
target_link_libraries(Week11-CodeProvided ${ASSIMP_LIB})
target_link_libraries(Week11-CodeProvided ${GLUT_LIB})
target_link_libraries(Week11-CodeProvided ${OPENGL_LIB})
target_link_libraries(Week11-CodeProvided ${GLEW_LIB})