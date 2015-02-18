
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <GLAER/glaer.h>
#include <GLFW/glfw3.h>

// TODO kill the glaer test code and do something

using namespace std;

GlaerContext ctx;

GlaerContext * current_glaer_context_impl() {
	return &ctx;
}

void draw_dummy(unsigned instances = 1) {
	static GLuint vao = 0;
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_POINTS, 0, 1, instances);
	glBindVertexArray(0);
}


class shader_error : public std::runtime_error {
public:
	explicit shader_error(const std::string &what_ = "Generic shader error.") : std::runtime_error(what_) { }
};

class shader_type_error : public shader_error {
public:
	explicit shader_type_error(const std::string &what_ = "Bad shader type.") : shader_error(what_) { }
};

class shader_compile_error : public shader_error {
public:
	explicit shader_compile_error(const std::string &what_ = "Shader compilation failed.") : shader_error(what_) { }
};

class shader_link_error : public shader_error {
public:
	explicit shader_link_error(const std::string &what_ = "Shader program linking failed.") : shader_error(what_) { }
};

inline void printShaderInfoLog(GLuint obj) {
	int infologLength = 0;
	int charsWritten = 0;
	glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1) {
		std::vector<char> infoLog(infologLength);
		glGetShaderInfoLog(obj, infologLength, &charsWritten, &infoLog[0]);
		cout << "SHADER:\n" << &infoLog[0];
	}
}

inline void printProgramInfoLog(GLuint obj) {
	int infologLength = 0;
	int charsWritten = 0;
	glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1) {
		std::vector<char> infoLog(infologLength);
		glGetProgramInfoLog(obj, infologLength, &charsWritten, &infoLog[0]);
		cout << "PROGRAM:\n" << &infoLog[0];
	}
}

inline GLuint compileShader(GLenum type, const std::string &text) {
	GLuint shader = glCreateShader(type);
	const char *text_c = text.c_str();
	glShaderSource(shader, 1, &text_c, nullptr);
	glCompileShader(shader);
	GLint compile_status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	if (!compile_status) {
		printShaderInfoLog(shader);
		throw shader_compile_error();
	}
	// always print, so we can see warnings
	printShaderInfoLog(shader);
	return shader;
}

inline void linkProgram(GLuint prog) {
	glLinkProgram(prog);
	GLint link_status;
	glGetProgramiv(prog, GL_LINK_STATUS, &link_status);
	if (!link_status) {
		printProgramInfoLog(prog);
		throw shader_link_error();
	}
	// always print, so we can see warnings
	printProgramInfoLog(prog);
}

inline GLuint makeProgram(const string &profile, const vector<GLenum> &stypes, const string &source) {
	GLuint prog = glCreateProgram();

	auto get_define = [](GLenum stype) {
		switch (stype) {
		case GL_VERTEX_SHADER:
			return "_VERTEX_";
		case GL_GEOMETRY_SHADER:
			return "_GEOMETRY_";
		case GL_TESS_CONTROL_SHADER:
			return "_TESS_CONTROL_";
		case GL_TESS_EVALUATION_SHADER:
			return "_TESS_EVALUATION_";
		case GL_FRAGMENT_SHADER:
			return "_FRAGMENT_";
		default:
			return "_DAMN_AND_BLAST_";
		}
	};

	for (auto stype : stypes) {
		ostringstream oss;
		oss << "#version " << profile << endl;
		oss << "#define " << get_define(stype) << endl;
		oss << source;
		auto shader = compileShader(stype, oss.str());
		glAttachShader(prog, shader);
	}

	linkProgram(prog);
	cout << "Shader program compiled and linked successfully" << endl;
	return prog;
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

	GLFWwindow* window;

	if (!glfwInit()) {
		abort();
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window) {
		glfwTerminate();
		abort();
	}

	glfwMakeContextCurrent(window);

	// tell glaer how to get its current context
	glaerSetCurrentContextProvider(current_glaer_context_impl);

	// initialize glaer context
	if (!glaerInitCurrentContext()) {
		abort();
	}

	cout << "GL version string: " << glGetString(GL_VERSION) << endl;

	// compile shader
	GLuint prog = makeProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, shader_prog_src);

	while (!glfwWindowShouldClose(window)) {
		
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		// render!
		glUseProgram(prog);
		draw_dummy();
		glUseProgram(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;

}