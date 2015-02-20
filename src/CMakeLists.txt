

# list headers here
SET(skadi_hdr
	"Camera.hpp"
	"GECom.hpp"
	"GL.hpp"
	"Graph.hpp"
	"GraphEditor.hpp"
	"Heightmap.hpp"
	"Initial3D.hpp"
	"Image.hpp"
	"Log.hpp"
	"Perlin.hpp"
	"RidgeConverter.hpp"
	"Window.hpp"
	"Concurrent.hpp"
	"Shader.hpp"
	"SimpleShader.hpp"
	"Brush.hpp"
)


# list sources here
SET(skadi_src
	"main.cpp"
	"Log.cpp"
	"Perlin.hpp"
	"Window.cpp"
	"Concurrent.cpp"
)


add_executable(skadi ${skadi_hdr} ${skadi_src})

target_link_libraries(skadi glfw ${GLFW_LIBRARIES})

add_definitions(${GLAER_DEFINITIONS})
target_link_libraries(skadi glaer ${GLAER_LIBRARIES})
