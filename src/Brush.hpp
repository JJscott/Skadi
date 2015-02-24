#pragma once

#include <vector>

#include "Initial3D.hpp"
#include "Graph.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

namespace skadi {

	class Brush {
	public:
		virtual void activate(const initial3d::vec3f &position, float radius, Graph *g, bool alt) {
			active = true;
			altClick = alt;
			step(position, radius, initial3d::vec3f(), g);
			onActivate(position, radius, g);
		}

		virtual void deactivate(const initial3d::vec3f &position, float radius, Graph *g) {
			active = false;
			altClick = false;
			onDeactivate(position, radius, g);
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) {  };

		bool isActive() { return active; }
		bool isAlt() { return altClick; }
	protected:

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) {  }
		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) {  }

		std::vector<Graph::Node *> getNodesInBrush(const initial3d::vec3f &position, float radius, Graph *g) {
			std::vector<Graph::Node *> nodes;
			for (Graph::Node * n : g->getNodes()) {
				if ((n->position - position).mag() <= radius ) {
					nodes.push_back(n);
				}
			}
			return nodes;
		}
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

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) {
			std::vector<Graph::Node *> nodes;
			for (Graph::Node * n : g->getNodes()) {
				if ((n->position - position).mag() <= radius ) {
					nodes.push_back(n);
					std::cout << "Node :: " << n->position << std::endl;
				}
			}
			std::cout << "CLICK STEP Alt=" << isAlt() << " : " << position << std::endl;
		}
	
	private:
		NullBrush() {}
	};


	class NodeBrush : public Brush {
	public:
		static NodeBrush * inst() {
			static NodeBrush *s = new NodeBrush();
			return s;
		}

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) {
			float e = 0, n = 0;
			for (Graph::Node *node : getNodesInBrush(position, radius, g)) {
				e += node->elevation;
				n++;
			}
			e /= n;
			temp_node = g->addNode(position, e);
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) {
			Graph::Node *n = nullptr;
			float distance = 10; //randomly high
			for (Graph::Node *node : getNodesInBrush(position, radius, g)) {
				if ( (node->position - position).mag() < distance ) {
					n = node;
				}
			}
			if (n != nullptr) {
				g->addEdge(temp_node, n);
			}
		}

	private:
		NodeBrush() {}
		Graph::Node *temp_node;
	};
}
