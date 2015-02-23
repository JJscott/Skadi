#pragma once

#include <vector>

#include "Initial3D.hpp"

#include "Graph.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

namespace skadi {

	class Brush {
	public:
		void activate(int dx, int dy, const std::vector<Graph::Node *> &nodes, Graph *g, bool alt) {
			active = true;
			altClick = alt;
			step(dx, dy, initial3d::vec3f(), nodes, g);
		}

		void deactivate() {
			active = false;
			altClick = false;
		}

		virtual void step(int dx, int dy, const initial3d::vec3f &node_distance, const std::vector<Graph::Node *> &nodes, Graph *g) = 0;

		bool isActive() { return active; }
		bool isAlt() { return altClick; }
	private:
		bool active = false;
		bool altClick = false;
	};


	class NullBrush : public Brush {
	public:
		static NullBrush * inst() {
			static NullBrush *n = new NullBrush();
			return n;
		}

		virtual void step(int dx, int dy, const initial3d::vec3f &node_distance, const std::vector<Graph::Node *> &nodes, Graph *g) {
			std::cout << "CLICK STEP :: " << dx << "," << dy << std::endl;
		}
	
	private:
		NullBrush() {}
	};


	class SelectBrush : public Brush {
	public:
		static SelectBrush * inst() {
			static SelectBrush *s = new SelectBrush();
			return s;
		}

		virtual void step(int dx, int dy, const initial3d::vec3f &node_distance, const std::vector<Graph::Node *> &nodes, Graph *g) {
			for (Graph::Node *n :  nodes) {
				n->selected = !isAlt();
			}
		}

	private:
		SelectBrush() {}
	};

}
