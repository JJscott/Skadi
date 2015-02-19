

# list headers here
SET(skadi_hdr
	"GECom.hpp"
	"GL.hpp"
	"Initial3D.hpp"
	"Log.hpp"
	"Window.hpp"
	"Concurrent.hpp"
	"Shader.hpp"
	"SimpleShader.hpp"
)


# list sources here
SET(skadi_src
	"main.cpp"
	"Log.cpp"
	"Window.cpp"
	"Concurrent.cpp"
)


add_executable(skadi ${skadi_hdr} ${skadi_src})

target_link_libraries(skadi glfw ${GLFW_LIBRARIES})

add_definitions(${GLAER_DEFINITIONS})
target_link_libraries(skadi glaer ${GLAER_LIBRARIES})
