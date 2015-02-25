#pragma once

#include <unordered_map>
#include <vector>
#include <memory>

#include "Camera.hpp"
#include "Graph.hpp"
#include "GL.hpp"
#include "Initial3D.hpp"
#include "SimpleShader.hpp"
#include "Log.hpp"
#include "RidgeConverter.hpp"

namespace skadi {

	class GraphEditor {
	public:

		GraphEditor(gecom::Window *win, Heightmap *hmap_) {
			graph = new Graph();
			camera = new EditorCamera(win, initial3d::vec3d(0.5, 0.5, 0.0), win->size().h * 0.9);
			window = win;
			hmap = hmap_;

			glGenFramebuffers(1, &fbo_graph);
			glGenTextures(1, &tex_graph);
			
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex_graph);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, graph_tex_width, graph_tex_width, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			// TODO could do mipmaps, but fuck it
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_graph);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_graph, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			// subscribe to events through this proxy
			// allows event dispatch to this 'component' to be enabled / disabled
			// using .forever() on a local subscription to this proxy is ok
			weproxy = std::make_shared<gecom::WindowEventProxy>();
			weproxy_sub = win->subscribeEventDispatcher(weproxy);

			brush = SelectBrush::inst();

			// Brush
			//
			brush_radius = 10;

			// Listen for mouse movement
			//
			weproxy->onMouseMove.subscribe([&](const gecom::mouse_event &e) {
				if (brush->isActive()) {
					// Move the brush
					initial3d::vec3f oldPos = brush_position;
					brush_position = initial3d::vec3f(e.pos.x, e.pos.y, 0);

					// Calculate relative to map
					initial3d::vec3f brush_pos_g = windowToGraph(brush_position);
					initial3d::vec3f brush_mov_g = brush_pos_g - windowToGraph(oldPos);
					float brush_rad_g = (brush_pos_g - windowToGraph(brush_position + ~initial3d::vec3f(1, 1, 0) * brush_radius)).mag();

					brush->step(brush_pos_g, brush_rad_g, brush_mov_g, graph);
				}
				brush_position = initial3d::vec3f(e.pos.x, e.pos.y, 0);
				return false; // Nessesary
			}).forever();

			// Listen for mouse click
			//
			weproxy->onMouseButtonPress.subscribe([&](const gecom::mouse_button_event &e) {
				if (!brush->isActive()) {
					// Calculate What nodes are in area
					initial3d::vec3f brush_pos_g = windowToGraph(brush_position);
					float brush_rad_g = (brush_pos_g - windowToGraph(brush_position + ~initial3d::vec3f(1, 1, 0) * brush_radius)).mag();

					if (e.button == GLFW_MOUSE_BUTTON_1) {
						brush->activate(brush_pos_g, brush_rad_g, graph, false);
					}else if (e.button == GLFW_MOUSE_BUTTON_2) {
						brush->activate(brush_pos_g, brush_rad_g, graph, true);
					}
				}
				return false; // Nessesary
			}).forever();

			weproxy->onMouseButtonRelease.subscribe([&](const gecom::mouse_button_event &e) {
				// Calculate What nodes are in area
				initial3d::vec3f brush_pos_g = windowToGraph(brush_position);
				float brush_rad_g = (brush_pos_g - windowToGraph(brush_position + ~initial3d::vec3f(1, 1, 0) * brush_radius)).mag();

				if (brush->isActive()) {
					if (e.button == GLFW_MOUSE_BUTTON_1 && !brush->isAlt()) {
						brush->deactivate(brush_pos_g, brush_rad_g, graph);
					}
					else if (e.button == GLFW_MOUSE_BUTTON_2 && brush->isAlt()) {
						brush->deactivate(brush_pos_g, brush_rad_g, graph);
					}
				}
				return false; // Nessesary
			}).forever();

			// Listen for key presses
			//
			weproxy->onKeyPress.subscribe([&](const gecom::key_event &e) {

				if (e.key >= GLFW_KEY_0 && e.key <= GLFW_KEY_9) {

					if (e.key == GLFW_KEY_1) brush = SelectBrush::inst();
					if (e.key == GLFW_KEY_2) brush = FixBrush::inst();
					if (e.key == GLFW_KEY_3) brush = MoveBrush::inst();
					if (e.key == GLFW_KEY_4) brush = ElevateBrush::inst();

					// TODO sharpness brush

					if (e.key == GLFW_KEY_6) brush = ConnectBrush::inst();
					if (e.key == GLFW_KEY_7) brush = NodeBrush::inst();

					std::cout << "Brush: " << brush->getName() << std::endl;
				}

				// clear selection
				if (e.key == GLFW_KEY_R) {
					graph->clearSelection();
				}

				// delete selection
				if (e.key == GLFW_KEY_DELETE) {
					std::unordered_set<Graph::Node *> sel = graph->getSelectedNodes();
					for (Graph::Node *n : sel) {
						graph->deleteNode(n);
					}
				}

				// make heightmap
				if (e.key == GLFW_KEY_H) {
					should_make_hmap = true;
				}

				// enable / disable layout
				if (e.key == GLFW_KEY_L) {
					should_do_layout = !should_do_layout;
				}

				// enable / disable automatic edge splitting and node branching
				if (e.key == GLFW_KEY_K) {
					should_expand_graph = !should_expand_graph;
				}

				return false;
			}).forever();

			// scroll for brush radius
			weproxy->onMouseScroll.subscribe([&](const gecom::mouse_scroll_event &e) {
				brush_radius += e.offset.h;
				return false;
			}).forever();

			Graph::Node *n1 = graph->addNode(initial3d::vec3f(0.5, 0.5, 0),   0.5, 0.0);
			//Graph::Node *n2 = graph->addNode(initial3d::vec3f(0.35, 0.1, 0),  0.7, 0.0);
			//Graph::Node *n3 = graph->addNode(initial3d::vec3f(0.2, 0.35, 0),  0.5, 0.0);
			//Graph::Node *n4 = graph->addNode(initial3d::vec3f(0.6, 0.42, 0),  0.7, 0.0);
			//Graph::Node *n5 = graph->addNode(initial3d::vec3f(0.86, 0.35, 0), 0.5, 0.0);
			//Graph::Node *n6 = graph->addNode(initial3d::vec3f(0.9, 0.5, 0),   0.7, 0.0);
			//Graph::Node *n7 = graph->addNode(initial3d::vec3f(0.42, 0.8, 0),  0.5, 0.0);
			//Graph::Node *n8 = graph->addNode(initial3d::vec3f(0.62, 0.9, 0),  0.7, 0.0);
			//Graph::Node *n9 = graph->addNode(initial3d::vec3f(0.76, 0.82, 0), 0.7, 0.0);

			n1->fixed = true;

			//graph->addEdge(n1, n2);
			//graph->addEdge(n1, n3);
			//graph->addEdge(n1, n4);
			//graph->addEdge(n4, n5);
			//graph->addEdge(n4, n6);
			//graph->addEdge(n1, n7);
			//graph->addEdge(n7, n8);
			//graph->addEdge(n8, n9);

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

			layout(location = 0) in vec4 pos_m;

			out VertexData {
				vec3 pos_m;
				flat bool selected;
				flat bool fixed;
			} vertex_out;

			void main() {
				vertex_out.pos_m = vec3(pos_m.x, pos_m.y, 0.9);
				uint flags = floatBitsToUint(pos_m.w);
				vertex_out.selected = bool(flags & 0x1u);
				vertex_out.fixed = bool(flags & 0x2u);
			}

			#endif

			#ifdef _GEOMETRY_

			layout(points) in;
			layout(triangle_strip, max_vertices = 8) out;

			in VertexData {
				vec3 pos_m;
				flat bool selected;
				flat bool fixed;
			} vertex_in[];

			out VertexData {
				vec3 pos_v;
				flat bool selected;
				flat bool fixed;
			} vertex_out;
			
			void main() {

				vec3 pos_v = (modelViewMatrix * vec4(vertex_in[0].pos_m, 1.0)).xyz;
				//vec3 pos_v = (vec4(vertex_in[0].pos_m, 1.0)).xyz;

				vertex_out.selected = vertex_in[0].selected;
				vertex_out.fixed = vertex_in[0].fixed;

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
				flat bool selected;
				flat bool fixed;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				if (vertex_in.selected) {
					frag_color = vec4(0.1, 0.8, 0.1, 1.0);
				} else if (vertex_in.fixed) {
					frag_color = vec4(0.0, 0.0, 0.0, 1.0);
				} else {
					frag_color = vec4(vec3(0.5), 1.0);
				}
			}

			#endif

			)delim";


			static const char *edge_shader_prog_src = R"delim(

			uniform mat4 modelViewMatrix;
			uniform mat4 projectionMatrix;
			uniform float elevationMax;

			#ifdef _VERTEX_

			layout(location = 0) in vec4 pos_m;

			out VertexData {
				float elevation;
			} vertex_out;

			void main() {
				vertex_out.elevation = pos_m.z;
				gl_Position = projectionMatrix * modelViewMatrix * vec4(pos_m.x, pos_m.y, 0.95, 1.0);
			}

			#endif

			#ifdef _FRAGMENT_

			in VertexData {
				float elevation;
			} vertex_in;

			out vec4 frag_color;

			void main() {
				float f = vertex_in.elevation / elevationMax;
				frag_color = vec4(f, 0.0, 1.0 - f, 1.0);
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

			// fire artifical mouse move events to keep brushes updated
			gecom::mouse_event mme;
			mme.window = window;
			mme.entered = false;
			mme.exited = false;
			glfwGetCursorPos(window->handle(), &mme.pos.x, &mme.pos.y);
			weproxy->dispatchMouseEvent(mme);

			if (should_do_layout) {
				if (doLayout() && should_expand_graph) {
					subdivideAndBranch();
				}
			}

			if (should_make_hmap) {
				makeHeightmap();
				should_make_hmap = false;
			}

		}

		void draw() {
						
			draw_graph();
			draw_brush();
			draw_box();

			// Finish
			//
			glUseProgram(0);
			glBindVertexArray(0);
		}

		initial3d::mat4f get_graph_proj_mat(float w, float h) {
			return initial3d::mat4f::scale(2.f / w, 2.f / h, 1);

		}

		void draw_graph(const initial3d::mat4f &view_mat, const initial3d::mat4f &proj_mat) {

			using namespace std;
			using namespace initial3d;

			

			// mat4d view_matrix = !camera->getViewTransform();

			vector<float> nodePos;
			vector<GLuint> edgeIdx;
			unordered_map<Graph::Node *, GLuint> nodeToIdx;

			float elevation_max = 0.f;

			for (Graph::Node *node : graph->getNodes()) {
				nodeToIdx[node] = nodePos.size() / 4;
				vec3f pos = node->position;
				nodePos.push_back(pos.x());
				nodePos.push_back(pos.y());
				nodePos.push_back(node->elevation);
				elevation_max = max(elevation_max, node->elevation);
				// bithacks - flags as bits in a float
				GLuint flags = 0;
				flags |= GLuint(node->selected) << 0;
				flags |= GLuint(node->fixed) << 1;
				nodePos.push_back(reinterpret_cast<float &>(flags));
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
			if (nodePos.empty()) {
				glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
			} else {
				glBufferData(GL_ARRAY_BUFFER, nodePos.size() * sizeof(float), &nodePos[0], GL_DYNAMIC_DRAW);
			}
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

			// Bind node VAO for Nodes
			//
			glBindVertexArray(vao_edge);

			// Upload indices
			//
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_edge_idx); // this sticks to the vao
			if (edgeIdx.empty()) {
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, 1 * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);
			} else {
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, edgeIdx.size() * sizeof(GLuint), &edgeIdx[0], GL_DYNAMIC_DRAW);
			}

			// Upload positions
			//
			glBindBuffer(GL_ARRAY_BUFFER, vbo_node_pos);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);


			

			//Actual Draw Calls
			//
			glUseProgram(shdr_node);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "modelViewMatrix"), 1, true, view_mat);
			glUniformMatrix4fv(glGetUniformLocation(shdr_node, "projectionMatrix"), 1, true, proj_mat);

			glBindVertexArray(vao_node);
			glDrawArrays(GL_POINTS, 0, nodePos.size() / 4);


			glUseProgram(shdr_edge);
			glUniformMatrix4fv(glGetUniformLocation(shdr_edge, "modelViewMatrix"), 1, true, view_mat);
			glUniformMatrix4fv(glGetUniformLocation(shdr_edge, "projectionMatrix"), 1, true, proj_mat);
			glUniform1f(glGetUniformLocation(shdr_edge, "elevationMax"), elevation_max);

			glBindVertexArray(vao_edge);
			glDrawElements(GL_LINES, edgeIdx.size(), GL_UNSIGNED_INT, nullptr);
		}

		void draw_graph() {
			using namespace initial3d;

			float h = window->size().h;
			float w = window->size().w;

			// Construct View and Projection Matricies
			mat4f proj_mat = get_graph_proj_mat(w, h);
			mat4f view_mat = camera->getViewTransform();

			draw_graph(view_mat, proj_mat);
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


		void draw_box() {

			using namespace initial3d;

			float h = window->size().h;
			float w = window->size().w;

			static const char * prog_box_src = R"delim(
			
			uniform mat4 modelViewMatrix;
			uniform mat4 projectionMatrix;

			#ifdef _VERTEX_

			void main() { }

			#endif

			#ifdef _GEOMETRY_

			layout(points) in;
			layout(line_strip, max_vertices = 5) out;

			void main() {
				
				const vec2[] p = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0), vec2(0.0, 0.0));
				
				for (int i = 0; i < p.length; i++) {
					gl_Position = projectionMatrix * modelViewMatrix * vec4(p[i], 0.99, 1.0);
					EmitVertex();
				}
				
			}

			#endif

			#ifdef _FRAGMENT_

			out vec4 frag_color;

			void main() {
				frag_color = vec4(vec3(0.75), 1.0); 
			}

			#endif

			)delim";

			static GLuint prog_box = 0;

			if (prog_box == 0) {
				prog_box = makeShaderProgram("330 core", { GL_VERTEX_SHADER, GL_GEOMETRY_SHADER, GL_FRAGMENT_SHADER }, prog_box_src);
			}

			mat4f proj_mat = get_graph_proj_mat(w, h);
			mat4f view_mat = camera->getViewTransform();

			glUseProgram(prog_box);
			glUniformMatrix4fv(glGetUniformLocation(prog_box, "modelViewMatrix"), 1, true, view_mat);
			glUniformMatrix4fv(glGetUniformLocation(prog_box, "projectionMatrix"), 1, true, proj_mat);
			gecom::draw_dummy();

		}

		GLuint makeGraphTexture() {

			using namespace initial3d;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_graph);
			glViewport(0, 0, graph_tex_width, graph_tex_width);

			glClearColor(1.f, 1.f, 1.f, 1.f);

			glClear(GL_COLOR_BUFFER_BIT);

			mat4f view_mat = mat4f::scale(graph_tex_width, graph_tex_width, 1) * mat4f::translate(-0.5f, -0.5f, 0);
			mat4f proj_mat = get_graph_proj_mat(graph_tex_width, graph_tex_width);

			draw_graph(view_mat, proj_mat);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			return tex_graph;

		}

		void makeHeightmap() {
			// just use mesh size for texture size
			int w = hmap->getMeshWidth();
			int h = hmap->getMeshHeight();
			// check for square and POT
			assert(w == h);
			assert((w != 0) && ((w & (w - 1)) == 0));
			// convert edges to heightmap
			gecom::log("Editor") << "Beginning heightmap creation...";
			std::vector<Graph::Edge *> edges(graph->getEdges().begin(), graph->getEdges().end());
			auto ele = RidgeConverter::ridgeToHeightmap(edges, w + 1);
			//hmap->setScale(initial3d::vec3d(5, 5, 5));
			hmap->setHeights(&ele[0], w + 1, w + 1);
			gecom::log("Editor") << "Heightmap creation finished";
		}

		void subdivideAndBranch() {

		}

		bool doLayout(const std::unordered_set<Graph::Node *> &active_nodes) {
			// rough complexity estimate
			float complexity = active_nodes.size() * log(graph->getNodes().size());
			int steps = 20000.f / complexity + 2.f;
			return graph->doLayout(steps, active_nodes) < steps;
		}

		bool doLayout() {
			if (graph->getSelectedNodes().empty()) {
				// nothing selected, layout all
				return doLayout(graph->getNodes());
			} else {
				// layout selection
				return doLayout(graph->getSelectedNodes());
			}
		}

		Graph * getGraph() { return graph; }

		Heightmap * getHeightmap() {
			return hmap;
		}

		bool enableEventDispatch(bool b) {
			if (weproxy_sub.enable(b)) {
				// e.g. generate synthetic focus gained / lost events
				return true;
			}
			return false;
		}

	private:
		
		// transform window coords to graph coords
		initial3d::vec3f windowToGraph(initial3d::vec3f mousePosition) {

			using namespace std;
			using namespace initial3d;

			int w = window->size().w;
			int h = window->size().h;
			initial3d::mat4f mat = !camera->getViewTransform() * !get_graph_proj_mat(w, h) * get_brush_proj_mat(w, h);

			// vec3f onscreenPos = mousePosition;
			// vec3f displayPos = get_brush_proj_mat(w, h) * onscreenPos;
			// vec3f nodePos = (!get_graph_proj_mat(w, h)) * displayPos;
			// vec3f scalePos = (!initial3d::mat4f::scale(size, size, 1)) * nodePos;

			// cout << "onscreenPos :: " << onscreenPos << endl;
			// cout << "displayPos :: " << displayPos << endl;
			// cout << "nodePos :: " << nodePos << endl;
			// cout << "scalePos :: " << scalePos << endl << " " << endl;

			return mat * mousePosition;
		};

		// window and event handling
		gecom::Window *window;
		std::shared_ptr<gecom::WindowEventProxy> weproxy;
		gecom::subscription weproxy_sub;

		//
		//
		Camera *camera;

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
		float brush_radius;
		initial3d::vec3f brush_position;

		GLuint shdr_brush;
		GLuint vao_brush;
		GLuint vbo_brush_angles;

		GLuint fbo_graph;
		GLuint tex_graph;

		static const int graph_tex_width = 2048;

		Heightmap *hmap;

		bool should_make_hmap = false;
		bool should_do_layout = false;
		bool should_expand_graph = false;

	};
}