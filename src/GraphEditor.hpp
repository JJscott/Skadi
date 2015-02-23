
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

		GraphEditor(gecom::Window *win, int s) {
			graph = new Graph();
			camera = new EditorCamera(win, initial3d::vec3d());
			window = win;
			size = s;

			brush = NullBrush::inst();

			// Brush
			//
			brush_radius = 10;

			// Listen for mouse movement
			//
			win->onMouseMove.subscribe([&](const gecom::mouse_event &e) {
				if (brush->isActive()) {
					// Move the brush
					//
					int w = window->size().w;
					int h = window->size().h;

					initial3d::vec3f oldPos = brush_position;
					initial3d::vec3f newPos = initial3d::vec3f(e.pos.x, e.pos.y, 0);
					brush_position = newPos;

					// Calculate relative to map
					//
					initial3d::vec3f relativePos = brushRelativePosition(brush_position);
					initial3d::vec3f movement = brushRelativePosition(newPos - oldPos);

					//Calculate What nodes are in area
					//
					std::vector<Graph::Node *> nodes;
					for (Graph::Node *n : graph->getNodes()) {
						if ((n->position - relativePos).mag() < brush_radius) {
							nodes.push_back(n);
						}
					}

					brush->step(relativePos, movement, nodes, graph);
				}
				brush_position = initial3d::vec3f(e.pos.x, e.pos.y, 0);
				return false; // Nessesary
			}).forever();

			// Listen for mouse click
			//
			win->onMousePress.subscribe([&](const gecom::mouse_button_event &e) {
				int w = window->size().w;
				int h = window->size().h;

				if (!brush->isActive()) {
					//Calculate What nodes are in area
					//
					initial3d::vec3f relativePos = brushRelativePosition(brush_position);
					std::vector<Graph::Node *> nodes;
					for (Graph::Node *n : graph->getNodes()) {
						if ((n->position - relativePos).mag() < brush_radius) {
							nodes.push_back(n);
						}
					}

					if (e.button == GLFW_MOUSE_BUTTON_1) {
						brush->activate(relativePos, nodes, graph, false);
					}else if (e.button == GLFW_MOUSE_BUTTON_2) {
						brush->activate(relativePos, nodes, graph, true);

					}
				}
				return false; // Nessesary
			}).forever();

			win->onMouseRelease.subscribe([&](const gecom::mouse_button_event &e) {
				if (brush->isActive()) {
					if (e.button == GLFW_MOUSE_BUTTON_1 && !brush->isAlt()) {
						brush->deactivate();
					}
					else if (e.button == GLFW_MOUSE_BUTTON_2 && brush->isAlt()) {
						brush->deactivate();
					}
				}
				return false; // Nessesary
			}).forever();

			// Listen for key presses
			//
			win->onKeyPress.subscribe([&](const gecom::key_event &e) {
				if (e.key == GLFW_KEY_LEFT_BRACKET) {
					brush_radius = brush_radius - 1;
				}
				else if (e.key == GLFW_KEY_RIGHT_BRACKET) {
					brush_radius = brush_radius + 1;
				}
				return false;
			}).forever();

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

			glGenVertexArrays(1, &vao_brush);
			glGenBuffers(1, &vbo_brush_angles);

			static const char *node_shader_prog_src = R"delim(

			uniform mat4 modelViewMatrix;
			uniform mat4 projectionMatrix;

			#ifdef _VERTEX_

			layout(location = 0) in vec2 pos_m;

			out VertexData {
				vec3 pos_m;
			} vertex_out;

			void main() {
				vertex_out.pos_m = vec3(pos_m.x, pos_m.y, 0.9);
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

				vec3 pos_v = (modelViewMatrix * vec4(vertex_in[0].pos_m, 1.0)).xyz;
				//vec3 pos_v = (vec4(vertex_in[0].pos_m, 1.0)).xyz;

				vertex_out.pos_v = pos_v + vec3(-2, -2, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3( 2, -2, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3(-2,  2, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				vertex_out.pos_v = pos_v + vec3( 2,  2, 0);
				gl_Position = projectionMatrix * vec4(vertex_out.pos_v, 1.0);
				EmitVertex();

				EndPrimitive();
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				vec3 pos_v;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				frag_color = vec4(1.0, 0.0, 0.0, 1.0); 
			}

			#endif

			)delim";


			static const char *edge_shader_prog_src = R"delim(

			uniform mat4 modelViewMatrix;
			uniform mat4 projectionMatrix;

			#ifdef _VERTEX_

			layout(location = 0) in vec2 pos_m;

			out VertexData {
				vec3 pos_m;
			} vertex_out;

			void main() {
				vertex_out.pos_m = (projectionMatrix * modelViewMatrix * vec4(pos_m.x, pos_m.y, 1.0, 1.0)).xyz;
				//vertex_out.pos_m = (projectionMatrix * vec4(pos_m.x, pos_m.y, 1.0, 1.0)).xyz;
				gl_Position = vec4(vertex_out.pos_m, 1);
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				vec3 pos_v;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				frag_color = vec4(0.0, 1.0, 0.0, 1.0); 
			}

			#endif

			)delim";

			static const char *brush_shader_prog_src = R"delim(

			uniform vec2 pos;
			uniform float radius;
			uniform mat4 proj_matrix;
			uniform float time;

			#ifdef _VERTEX_

			layout(location=0) in float angle;

			out VertexData {
				float angle;
			} vertex_out;

			void main() {
				vertex_out.angle = angle;
				gl_Position = proj_matrix * vec4(pos + radius * vec2(cos(angle), sin(angle)), 0.0, 1.0);
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				float angle;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				const float a = 7.0;
				float f = mix(0.0, 1.0, mod((time * 0.25 + vertex_in.angle) * radius, a) > 0.5 * a);
				frag_color = vec4(vec3(f), 1.0);
			}

			#endif

			)delim";

			shdr_node = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, node_shader_prog_src);
			shdr_edge = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, edge_shader_prog_src);
			shdr_brush = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER }, brush_shader_prog_src);

		}

		void update() {

			//update brush stuff here
			//

			camera->update();

		}

		void draw() {
			draw_graph();
			draw_brush();

			// Finish
			//
			glUseProgram(0);
			glBindVertexArray(0);
		}

		initial3d::mat4f get_graph_proj_mat(float w, float h) {
			return initial3d::mat4f::translate(initial3d::vec3f(-1, -1, 0)) * initial3d::mat4f::scale(1 / float(w / 2), 1 / float(h / 2), 1);

		}

		void draw_graph() {

			using namespace std;
			using namespace initial3d;

			float h = window->size().h;
			float w = window->size().w;

			// mat4d view_matrix = !camera->getViewTransform();

			vector<float> nodePos;
			vector<GLuint> edgeIdx;
			unordered_map<Graph::Node *, GLuint> nodeToIdx;

			for (Graph::Node *node : graph->getNodes()) {
				nodeToIdx[node] = nodePos.size() / 2;
				vec3f pos = node->position;
				nodePos.push_back(size * pos.x());
				nodePos.push_back(size * pos.z());
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


			// Construct View and Projection Matricies
			//
			mat4f proj_mat = get_graph_proj_mat(w, h);

			mat4f view_mat = camera->getViewTransform();


			//Actual Draw Calls
			//
			glUseProgram(shdr_node);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "modelViewMatrix"), 1, true, view_mat);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "projectionMatrix"), 1, true, proj_mat);

			glBindVertexArray(vao_node);
			glDrawArrays(GL_POINTS, 0, nodePos.size() / 2);


			glUseProgram(shdr_edge);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "modelViewMatrix"), 1, true, view_mat);
			glUniformMatrix4fv(glGetUniformLocation(shdr_edge, "projectionMatrix"), 1, true, proj_mat);

			glBindVertexArray(vao_edge);
			glDrawElements(GL_LINES, edgeIdx.size(), GL_UNSIGNED_INT, nullptr);
		}

		initial3d::mat4f get_brush_proj_mat(float w, float h) {
			return initial3d::mat4f::scale(1, -1, 1) * initial3d::mat4f::translate(-1, -1, 0) * initial3d::mat4f::scale(2.f / w, 2.f / h, 1);

		}

		void draw_brush() {

			using namespace std;
			using namespace initial3d;

			float w = window->size().w;
			float h = window->size().h;

			//Draw brush
			//
			mat4f proj_mat = get_brush_proj_mat(w, h);

			glBindVertexArray(vao_brush);

			std::vector<float> angles;

			for (int i = 0; i < 1000; i++) {
				float a = 2.0 * initial3d::math::pi() * i / (1000.0);
				angles.push_back(a);
			}

			glBindVertexArray(vao_brush);
			glBindBuffer(GL_ARRAY_BUFFER, vbo_brush_angles);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float)* angles.size(), &angles[0], GL_STATIC_DRAW);
			glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
			glEnableVertexAttribArray(0);

			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glUseProgram(shdr_brush);

			glUniform2f(glGetUniformLocation(shdr_brush, "pos"), brush_position.x(), brush_position.y());
			glUniform1f(glGetUniformLocation(shdr_brush, "radius"), brush_radius);
			glUniformMatrix4fv(glGetUniformLocation(shdr_brush, "proj_matrix"), 1, true, proj_mat);
			glUniform1f(glGetUniformLocation(shdr_brush, "time"), fmod(glfwGetTime(), initial3d::math::pi() * 32.0));

			glDrawArrays(GL_LINE_LOOP, 0, 1000);
		}

		Graph *getGraph() { return graph; }


	private:
		
		initial3d::vec3f brushRelativePosition(initial3d::vec3f mouseMovement) {
			int w = window->size().w;
			int h = window->size().h;
			initial3d::mat4f mat = (!get_graph_proj_mat(w, h)) * get_brush_proj_mat(w, h);
			return mat * mouseMovement;
		};

		//
		//
		Camera *camera;
		gecom::Window *window;
		int size;

		// Graph
		//
		Graph *graph;

		GLuint vao_node;
		GLuint vao_edge;
		GLuint vbo_node_pos;
		GLuint ibo_edge_idx;
		GLuint shdr_node;
		GLuint shdr_edge;

		// Brush
		//
		Brush *brush;
		int brush_radius;
		initial3d::vec3f brush_position;

		GLuint shdr_brush;
		GLuint vao_brush;
		GLuint vbo_brush_angles;

	};
}