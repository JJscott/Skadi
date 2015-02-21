
#include <unordered_map>
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
			graph = new Graph();

			Graph::Node *n1 = graph->addNode(initial3d::vec3f(0.4, 0.5, 0.4));
			Graph::Node *n2 = graph->addNode(initial3d::vec3f(0.35, 0.7, 0.1));
			Graph::Node *n3 = graph->addNode(initial3d::vec3f(0.2, 0.5, 0.35));
			Graph::Node *n4 = graph->addNode(initial3d::vec3f(0.6, 0.7, 0.42));
			Graph::Node *n5 = graph->addNode(initial3d::vec3f(0.86, 0.5, 0.35));
			Graph::Node *n6 = graph->addNode(initial3d::vec3f(0.9, 0.7, 0.5));
			Graph::Node *n7 = graph->addNode(initial3d::vec3f(0.42, 0.5, 0.8));
			Graph::Node *n8 = graph->addNode(initial3d::vec3f(0.62, 0.7, 0.9));
			Graph::Node *n9 = graph->addNode(initial3d::vec3f(0.76, 0.7, 0.82));

			graph->addEdge(n1, n2);
			graph->addEdge(n1, n3);
			graph->addEdge(n1, n4);
			graph->addEdge(n4, n5);
			graph->addEdge(n4, n6);
			graph->addEdge(n1, n7);
			graph->addEdge(n7, n8);
			graph->addEdge(n8, n9);

			glGenVertexArrays(1, &vao_node);
			glGenVertexArrays(1, &vao_edge);
			glGenBuffers(1, &vbo_node_pos);
			glGenBuffers(1, &ibo_edge_idx);


			static const char *shader_prog_src = R"delim(

			//uniform mat4 modelViewMatrix;
			uniform mat4 projectionMatrix;

			#ifdef _VERTEX_

			layout(location = 0) in vec2 pos_m;

			out VertexData {
				vec3 pos_m;
			} vertex_out;

			void main() {
				vertex_out.pos_m = vec3(pos_m.x, pos_m.y, 0.5);
			}

			#endif

			#ifdef _GEOMETRY_

			layout(points) in;
			layout(triangle_strip, max_vertices = 4) out;

			in VertexData {
				vec3 pos_m;
			} vertex_in[];

			out VertexData {
				vec3 pos_v;
			} vertex_out;

			void main() {

				//vec3 pos_v = (modelViewMatrix * vec4(vertex_in[0].pos_m, 1.0)).xyz;
				vec3 pos_v = (vec4(vertex_in[0].pos_m, 1.0)).xyz;

				vertex_out.pos_v = pos_v + vec3(-0.01, -0.01, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3( 0.01, -0.01, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3(-0.01,  0.01, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3( 0.01,  0.01, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				EndPrimitive();
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				vec3 pos_v;
			} vertex_in;

			void main() {
				gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); 
			}

			#endif

			)delim";

			shdr_node = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, shader_prog_src);


		}

		void update() {

		}

		void draw() {

			std::vector<float> nodePos;
			std::vector<GLuint> edgeIdx;
			std::unordered_map<Graph::Node *, GLuint> nodeToIdx;

			for (Graph::Node *node : graph->getNodes()) {
				nodeToIdx[node] = nodePos.size()/2;
				initial3d::vec3f pos = node->getPosition();
				nodePos.push_back(pos.x());
				nodePos.push_back(pos.z());
			}

			for (Graph::Edge *edge : graph->getEdges()) {
				edgeIdx.push_back(nodeToIdx[edge->getNode1()]);
				edgeIdx.push_back(nodeToIdx[edge->getNode2()]);
			}

			// Bind node VAO for Nodes
			//
			glBindVertexArray(vao_node);

			// Upload positions
			//
			glBindBuffer(GL_ARRAY_BUFFER, vbo_node_pos);
			glBufferData(GL_ARRAY_BUFFER, nodePos.size() * sizeof(float), &nodePos[0], GL_STATIC_DRAW); // Only upload once
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

			// Bind node VAO for Nodes
			//
			glBindVertexArray(vao_edge);

			// Upload indices
			//
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_edge_idx); // this sticks to the vao
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, edgeIdx.size() * sizeof(GLuint), &edgeIdx[0], GL_STATIC_DRAW);

			// Upload positions
			//
			glBindBuffer(GL_ARRAY_BUFFER, vbo_node_pos);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);


			//Actual Draw Calls
			glUseProgram(shdr_node);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "projectionMatrix"), 1, true, initial3d::mat4f(1));

			glBindVertexArray(vao_node);
			glDrawArrays(GL_POINTS, 0, nodePos.size() / 2);


			//glBindVertexArray(vao_edge);
			//glDrawElements(GL_LINES, edgeIdx.size(), GL_UNSIGNED_INT, nullptr);



			//Draw brush
			//


			// Finish
			//
			glBindVertexArray(0);
		}

		Graph * getGraph() { return graph; }


	private:
		Graph *graph;

		// GL information
		//
		GLuint vao_node;
		GLuint vao_edge;
		GLuint vbo_node_pos;
		GLuint ibo_edge_idx;
		GLuint shdr_node;
		GLuint shdr_edge;

	};
}