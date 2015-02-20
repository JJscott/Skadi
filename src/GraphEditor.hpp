

#include <vector>

#include "Camera.hpp"
#include "Graph.hpp"
#include "GL.hpp"
#include "Initial3D.hpp"
#include "SimpleShader.hpp"


namespace skadi {

	class GraphEditor {
	public:

		GraphEditor() {

		}

		void update() {

		}

		void draw() {
			for (Graph::Node *node : graph->getNodes()) {

			}

			for (Graph::Edge *edge : graph->getEdges()) {

			}



			//glGenVertexArrays(1, &vao);
			//glBindVertexArray(vao);
			//glGenBuffers(1, &ibo);
			//glGenBuffers(1, &vbo_pos);
			//glGenBuffers(1, &vbo_uv);

			//// upload indices
			//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo); // this sticks to the vao
			//glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), &idx[0], GL_STATIC_DRAW);

			//// upload positions
			//glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
			//glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(float), &pos[0], GL_STATIC_DRAW);
			//glEnableVertexAttribArray(0);
			//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);


			//Draw brush
			//
		}

		Graph * getGraph() { return graph; }


	private:
		Graph *graph;
		GLuint node_vao;
		GLuint edge_vao;
		GLuint node_pos_vbo;

	};
}