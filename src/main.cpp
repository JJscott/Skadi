
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "Window.hpp"
#include "SimpleShader.hpp"

using namespace std;

void draw_dummy(unsigned instances = 1) {
	static GLuint vao = 0;
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_POINTS, 0, 1, instances);
	glBindVertexArray(0);
}


const char *shader_prog_src = R"delim(

#ifdef _VERTEX_

void main() { }

#endif

#ifdef _GEOMETRY_

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

out vec2 texCoord;

void main() {
	gl_Position = vec4(3.0, 1.0, 0.0, 1.0);
	texCoord = vec2(2.0, 1.0);
	EmitVertex();
	
	gl_Position = vec4(-1.0, 1.0, 0.0, 1.0);
	texCoord = vec2(0.0, 1.0);
	EmitVertex();
	
	gl_Position = vec4(-1.0, -3.0, 0.0, 1.0);
	texCoord = vec2(0.0, -1.0);
	EmitVertex();
	
	EndPrimitive();
}

#endif

#ifdef _FRAGMENT_

in vec2 texCoord;
out vec4 frag_color;

void main() {
	frag_color = vec4(texCoord, 0.0, 1.0);
}

#endif

)delim";


int main() {

	// randomly placed note about texture parameters and debug messages:
	// nvidia uses this as mipmap allocation hint; not doing it causes warning spam
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	gecom::Window *win = gecom::createWindow().size(1024, 768).title("Skadi").visible(true);
	win->makeContextCurrent();

	// compile shader
	GLuint prog = skadi::makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, shader_prog_src);

	while (!win->shouldClose()) {
		
		auto size = win->size();
		glViewport(0, 0, size.w, size.h);

		// render!
		glUseProgram(prog);
		draw_dummy();
		glUseProgram(0);

		win->swapBuffers();
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;

}