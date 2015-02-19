
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include "Camera.hpp"
#include "Heightmap.hpp"
#include "Graph.hpp"
#include "RidgeConverter.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

using namespace std;
using namespace skadi;
using namespace initial3d;

void draw_dummy(unsigned instances = 1) {
	static GLuint vao = 0;
	if (vao == 0) {
		glGenVertexArrays(1, &vao);
	}
	glBindVertexArray(vao);
	glDrawArraysInstanced(GL_POINTS, 0, 1, instances);
	glBindVertexArray(0);
}


gecom::Window *win;
skadi::Projection *projection;
skadi::Camera *camera;


void draw_test_heightmap(initial3d::mat4f worldViewMat, initial3d::mat4f projMat) {
	static int size =  513;
	static Heightmap *hm = nullptr;


	if (hm == nullptr) {
		vector<Edge *> v;

		Node *n1 = new Node(vec3f(0.4, 0.5, 0.4));
		Node *n2 = new Node(vec3f(0.35, 0.7, 0.1));
		Node *n3 = new Node(vec3f(0.2, 0.5, 0.35));
		Node *n4 = new Node(vec3f(0.6, 0.7, 0.42));
		Node *n5 = new Node(vec3f(0.86, 0.5, 0.35));
		Node *n6 = new Node(vec3f(0.9, 0.7, 0.5));
		Node *n7 = new Node(vec3f(0.42, 0.5, 0.8));
		Node *n8 = new Node(vec3f(0.62, 0.7, 0.9));
		Node *n9 = new Node(vec3f(0.76, 0.7, 0.82));

		v.push_back(new Edge(n1, n2));
		v.push_back(new Edge(n1, n3));
		v.push_back(new Edge(n1, n4));
		v.push_back(new Edge(n4, n5));
		v.push_back(new Edge(n4, n6));
		v.push_back(new Edge(n1, n7));
		v.push_back(new Edge(n7, n8));
		v.push_back(new Edge(n8, n9));

		auto ele = RidgeConverter::ridgeToHeightmap(v, size);

		hm = new Heightmap(2048, 2048);
		hm->setScale(initial3d::vec3d(5, 1, 5));
		hm->setHeights(ele, size, size);
	}
	hm->draw(worldViewMat, projMat);
}

void display(int w, int h) {
	float zfar = 20000.0f;

	projection->setPerspectiveProjection(math::pi() / 3, double(w) / h, 0.1, zfar);
	camera->update();

	mat4d proj_matrix = projection->getProjectionTransform();
	mat4d view_matrix = !camera->getViewTransform();


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	draw_test_heightmap(view_matrix, proj_matrix);
	glFinish();

}



int main() {

	// randomly placed note about texture parameters and debug messages:
	// nvidia uses this as mipmap allocation hint; not doing it causes warning spam
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	win = gecom::createWindow().size(1024, 768).title("Skadi").visible(true);
	win->makeContextCurrent();

	// // listen for mouse movement
	// win->onMouseMove.subscribe([](const gecom::mouse_event &e) {
	// 	//cout << e.pos.x << " " << e.pos.y << endl;
	// 	return false;
	// }).forever();

	// // listen for key presses
	// win->onKeyPress.subscribe([](const gecom::key_event &e) {
	// 	//cout << e.key << endl;
	// 	return false;
	// }).forever();

	projection = new Projection();
	camera = new FPSCamera(win, vec3d(0, 0, 3), 0, 0 );

	double lastFPSTime = glfwGetTime();
	int fps = 0;

	while (!win->shouldClose()) {
		double now = glfwGetTime();
		auto size = win->size();
		glViewport(0, 0, size.w, size.h);

		// render!
		if ( size.w !=0 && size.h != 0)
			display(size.w, size.h);


		win->swapBuffers();
		glfwPollEvents();

		if (now - lastFPSTime > 1) {
			char fpsString[200];
			sprintf(fpsString, "Skiro [%d FPS @%dx%d]", fps, win->width(), win->height());
			win->title(fpsString);
			fps = 0;
			lastFPSTime = now;
		}
		fps++;
	}

	glfwTerminate();
	return 0;
}