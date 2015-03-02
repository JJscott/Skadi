
/*

editor controls, probably:
- 0-9: set brush
- WASD: move
- R: clear selection
- DEL: delete selection
- L: toggle layout on selection or everything
- K: toggle automatic graph subdivision and branching (not implemented yet)
- H: make heightmap

global:
- TAB: switch views
- F1: toggle graph texture on mesh

*/



#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <thread>

#include <omp.h>

#include "Brush.hpp"
#include "Camera.hpp"
#include "Heightmap.hpp"
#include "Graph.hpp"
#include "GraphEditor.hpp"
#include "RidgeConverter.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

using namespace std;
using namespace skadi;
using namespace initial3d;


gecom::Window *win;
skadi::Projection *projection;
skadi::Camera *camera;
skadi::GraphEditor *graphEditor;
skadi::Heightmap *heightmap;


//void draw_test_heightmap(initial3d::mat4f worldViewMat, initial3d::mat4f projMat) {
//	static int size =  513;
//	static Heightmap *hm = nullptr;
//
//	if (hm == nullptr) {
//		Graph *g = new Graph();
//
//		Graph::Node *n1 = g->addNode(vec3f(0.4, 0.5, 0.4));
//		Graph::Node *n2 = g->addNode(vec3f(0.35, 0.7, 0.1));
//		Graph::Node *n3 = g->addNode(vec3f(0.2, 0.5, 0.35));
//		Graph::Node *n4 = g->addNode(vec3f(0.6, 0.7, 0.42));
//		Graph::Node *n5 = g->addNode(vec3f(0.86, 0.5, 0.35));
//		Graph::Node *n6 = g->addNode(vec3f(0.9, 0.7, 0.5));
//		Graph::Node *n7 = g->addNode(vec3f(0.42, 0.5, 0.8));
//		Graph::Node *n8 = g->addNode(vec3f(0.62, 0.7, 0.9));
//		Graph::Node *n9 = g->addNode(vec3f(0.76, 0.7, 0.82));
//
//
//		g->addEdge(n1, n2);
//		g->addEdge(n1, n3);
//		g->addEdge(n1, n4);
//		g->addEdge(n4, n5);
//		g->addEdge(n4, n6);
//		g->addEdge(n1, n7);
//		g->addEdge(n7, n8);
//		g->addEdge(n8, n9);
//
//		std::vector<Graph::Edge *> edges(g->getEdges().begin(), g->getEdges().end());
//		auto ele = RidgeConverter::ridgeToHeightmap(edges, size);
//
//		hm = new Heightmap(512, 512);
//		hm->setScale(initial3d::vec3d(5, 5, 5));
//		hm->setHeights(&ele[0], size, size);
//	}
//
//	hm->draw(worldViewMat, projMat);
//}

void display(int w, int h, bool textured = false) {
	float zfar = 20000.0f;

	projection->setPerspectiveProjection(math::pi() / 3, double(w) / h, 0.1, zfar);
	camera->update();

	mat4d proj_matrix = projection->getProjectionTransform();
	mat4d view_matrix = camera->getViewTransform();

	glClearColor(1.f, 1.f, 1.f, 1.f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	GLuint tex = textured ? graphEditor->makeGraphTexture() : 0;

	glViewport(0, 0, w, h);

	heightmap->setScale(initial3d::vec3d(5, 5, 5));

	//draw_test_heightmap(view_matrix, proj_matrix);
	heightmap->draw(view_matrix, proj_matrix, tex);

	glFinish();

}

void displayEditor(int w, int h) {
	graphEditor->update();

	glClearColor(1.f, 1.f, 1.f, 1.f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// draw graphEditor here
	//
	graphEditor->draw();

	glFinish();

}




int main() {

	std::cout << std::boolalpha;

	RidgeConverter::test_shit();

	if (std::thread::hardware_concurrency()) {
		// leave a thread for UI
		unsigned tc = max(std::thread::hardware_concurrency() - 1, 1u);
		omp_set_num_threads(tc);
		gecom::log("OMP") << "Default thread count: " << tc;
	}


	const int size = 128;

	// randomly placed note about texture parameters and debug messages:
	// nvidia uses this as mipmap allocation hint; not doing it causes warning spam
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	win = gecom::createWindow().size(1024, 768).hint(GLFW_SAMPLES, 16).title("Skadi").visible(true);
	win->makeContextCurrent();

	bool editor_enabled = true;
	bool textured_mesh = true;

	win->onKeyPress.subscribe([&](const gecom::key_event &e) {
		if (e.key == GLFW_KEY_TAB) {
			// switch UIs by hackyness
			graphEditor->enableEventDispatch(!editor_enabled);
			editor_enabled = !editor_enabled;
		}

		if (e.key == GLFW_KEY_F1) {
			textured_mesh = !textured_mesh;
		}

		return false;
	}).forever();

	projection = new Projection();
	camera = new FPSCamera(win, vec3d(0, 0, 3), 0, 0 );
	heightmap = new Heightmap(size, size);
	graphEditor = new GraphEditor(win, heightmap);

	double lastFPSTime = glfwGetTime();
	int fps = 0;

	while (!win->shouldClose()) {
		glfwPollEvents();

		double now = glfwGetTime();
		auto size = win->size();
		glViewport(0, 0, size.w, size.h);

		// render!
		if (size.w != 0 && size.h != 0) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			if (editor_enabled) {
				displayEditor(size.w, size.h);
			} else {
				display(size.w, size.h, textured_mesh);
			}
		}


		win->swapBuffers();

		if (now - lastFPSTime > 1) {
			char fpsString[200];
			sprintf(
				fpsString, "Skadi [%d FPS @%dx%d] [%d/%d Nodes, %d SPS]",
				fps, win->width(), win->height(), graphEditor->getActiveNodeCount(),
				graphEditor->getGraph()->getNodes().size(), graphEditor->pollStepCount()
			);
			win->title(fpsString);
			fps = 0;
			lastFPSTime = now;
		}
		fps++;
	}

	delete win;

	glfwTerminate();
	return 0;
}