#pragma once

#include <vector>

#include "Initial3D.hpp"
#include "Graph.hpp"
#include "Window.hpp"
#include "SimpleShader.hpp"

namespace skadi {

	class Brush {
	public:
		void activate(const initial3d::vec3f &position, float radius, Graph *g, bool alt) {
			active = true;
			altClick = alt;
			onActivate(position, radius, g);
			step(position, radius, initial3d::vec3f(), g);
		}

		void deactivate(const initial3d::vec3f &position, float radius, Graph *g) {
			active = false;
			altClick = false;
			onDeactivate(position, radius, g);
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) {  };

		virtual const char * getName() = 0;

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

		Graph::Node * getClosestNodeInBrush(const initial3d::vec3f &position, float radius, Graph *g, Graph::Node *exclude = nullptr) {
			Graph::Node *n = nullptr;
			float distance = radius; // NOT RANDOM
			for (Graph::Node *node : g->getNodes()) {
				if (node == exclude) continue;
				float newDis = (node->position - position).mag();
				if (newDis < distance) {
					n = node;
					distance = newDis;
				}
			}
			return n;
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

		virtual const char * getName() override {
			return "Null";
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
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

		virtual const char * getName() override {
			return "Node";
		}

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			float e = 0, n = 1;
			for (Graph::Node *node : getNodesInBrush(position, radius, g)) {
				e += node->elevation;
				n++;
			}
			e /= n;
			temp_node = g->addNode(position, e);
			temp_node->fixed = true;
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			std::cout << position << std::endl;
			Graph::Node *n = getClosestNodeInBrush(position, radius, g, temp_node);
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


	// select
	class SelectBrush : public Brush {
	public:
		static SelectBrush * inst() {
			static SelectBrush *s = new SelectBrush();
			return s;
		}

		virtual const char * getName() override {
			return "Select";
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			for (Graph::Node * n : getNodesInBrush(position, radius, g)) {
				g->select(n, !isAlt());
			}
		};

	private:
		SelectBrush() { }
	};

	// fix
	class FixBrush : public Brush {
	public:
		static FixBrush * inst() {
			static FixBrush *s = new FixBrush();
			return s;
		}

		virtual const char * getName() override {
			return "Fix";
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			for (Graph::Node * n : getNodesInBrush(position, radius, g)) {
				n->fixed = !isAlt();
			}
		};

	private:
		FixBrush() { }
	};

	// move
	class MoveBrush : public Brush {
	public:
		static MoveBrush * inst() {
			static MoveBrush *s = new MoveBrush();
			return s;
		}

		virtual const char * getName() override {
			return "Move";
		}

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_nodes = getNodesInBrush(position, radius, g);
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_nodes.clear();
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			for (Graph::Node * n : temp_nodes) {
				n->position += travel_distance;
			}
		};

	private:
		MoveBrush() { }
		std::vector<Graph::Node *> temp_nodes;
	};

	// elevation
	class ElevateBrush : public Brush {
	public:
		static ElevateBrush * inst() {
			static ElevateBrush *s = new ElevateBrush();
			return s;
		}

		virtual const char * getName() override {
			return "Elevate";
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_nodes.clear();
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			// TODO brush strength type shit?
			static const float delta = 0.01f;
			auto nodes0 = getNodesInBrush(position, radius, g);
			for (Graph::Node *n : nodes0) {
				if (temp_nodes.find(n) != temp_nodes.end()) continue;
				n->elevation += isAlt() ? -delta : delta;
			}
			temp_nodes.clear();
			temp_nodes.insert(nodes0.begin(), nodes0.end());
		};

	private:
		ElevateBrush() { }
		std::unordered_set<Graph::Node *> temp_nodes;
	};

	// TODO sharpness


	// connect
	class ConnectBrush : public Brush {
	public:
		static ConnectBrush * inst() {
			static ConnectBrush *s = new ConnectBrush();
			return s;
		}

		virtual const char * getName() override {
			return "Connect";
		}

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_nodes = getNodesInBrush(position, radius, g);
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_nodes.clear();
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			for (Graph::Node * n : getNodesInBrush(position, radius, g)) {
				for (Graph::Node *n0 : temp_nodes) {
					// don't add reflexive edges; this breaks the heightmap conversion
					if (n0 == n) continue;
					if (isAlt()) {
						Graph::Edge *e = n0->findEdge(n);
						if (e) g->deleteEdge(e);
					} else {
						g->addEdge(n0, n);
					}
				}
			}
		};

	private:
		ConnectBrush() { }
		std::vector<Graph::Node *> temp_nodes;
	};

	// make a line of nodes
	class NodeLineBrush : public Brush {
	public:
		static NodeLineBrush * inst() {
			static NodeLineBrush *s = new NodeLineBrush();
			return s;
		}

		virtual const char * getName() override {
			return "NodeLine";
		}

		virtual void onActivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_node = getClosestNodeInBrush(position, radius, g);
			if (!temp_node) {
				temp_node = g->addNode(position, 0.f);
				temp_node->fixed = true;
			}
		}

		virtual void onDeactivate(const initial3d::vec3f &position, float radius, Graph *g) override {
			temp_node = nullptr;
		}

		virtual void step(const initial3d::vec3f &position, float radius, const initial3d::vec3f &travel_distance, Graph *g) override {
			if ((position - temp_node->position).mag() > radius) {
				Graph::Node *old_node = temp_node;
				temp_node = g->addNode(position, old_node->elevation);
				g->addEdge(old_node, temp_node);
			}
		};

	private:
		NodeLineBrush() { }
		Graph::Node *temp_node;
	};

}
