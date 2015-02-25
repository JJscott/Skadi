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
			onActivate(position, radius, g);
			step(position, radius, initial3d::vec3f(), g);
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
			float e = 0, n = 1;
			for (Graph::Node *node : getNodesInBrush(position, radius, g)) {
				e += node->elevation;
				n++;
			}
			e /= n;
			temp_node = g->addNode(position, e);
			temp_node->fixed = true;
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) {
			std::cout << position << std::endl;
			Graph::Node *n = nullptr;
			float distance = radius; // NOT RANDOM
			for (Graph::Node *node : g->getNodes()) {
				if (node == temp_node) continue;
				float newDis = (node->position - position).mag();
				if (newDis < distance) {
					n = node;
					distance = newDis;
				}
			}
			if (n != nullptr) {
				g->addEdge(temp_node, n);
				temp_node->fixed = false;
			}
			temp_node = nullptr;
		}

	private:
		NodeBrush() {}
		Graph::Node *temp_node = nullptr;
	};



	class SelectBrush : public Brush {
	public:
		static SelectBrush * inst() {
			static SelectBrush *s = new SelectBrush();
			return s;
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) {
			for (Graph::Node * n : getNodesInBrush(position, radius, g)) {
				g->select(n, !isAlt());
			}
		};

	private:
		SelectBrush() {}
		
	};
}
