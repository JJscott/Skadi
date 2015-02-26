#pragma once


#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>
#include <random>


#include "Initial3D.hpp"
#include "Float3.hpp"


namespace skadi {

	class Graph {
	public:

		class Node;
		class Edge;

		class Node {
		public:
			Node(initial3d::vec3f pos, float ele = 0, float sharp = 0) : position(pos.x(), pos.y(), 0), elevation(ele), sharpness(sharp) {  }

			const std::unordered_set<Edge *> & getEdges() const { return edges; }
			void addEdge(Edge *e) { edges.emplace(e); }
			void removeEdge(Edge *e) { edges.erase(e); }
			bool containsEdge(Edge *e) const { return edges.find(e) != edges.end(); }

			Edge * findEdge(Node *n) {
				for (Edge *e : edges) {
					if (e->other(this) == n) return e;
				}
				return nullptr;
			}

			initial3d::float3 position; //values of [0,1]

			// for layout
			initial3d::float3 velocity;
			float mass = 1.f;
			float charge = 1.f;

			float elevation = 0.f;
			float sharpness = 0.f;
			bool selected = false;
			
			// is the node locked in place
			bool fixed = false;

			float split_priority() {
				return 1;
			}

			float branch_priority() {
				// i think random branching works at least as well as any prioritzation ive come up with
				static std::default_random_engine rand;
				return std::uniform_real_distribution<float>(0.f, 1.f)(rand);
			}

		private:
			std::unordered_set<Edge *> edges;

			friend class Graph;
		};

		class Edge {
		public:

			Node * other(Node *n) const {
				// ben-certified
				assert(n == node1 || n == node2);
				return reinterpret_cast<Node *>(uintptr_t(node1) ^ uintptr_t(node2) ^ uintptr_t(n));
			}

			Node * getNode1() { return node1; }
			Node * getNode2() { return node2; }

			Node *node1;
			Node *node2;

			// for layout
			float spring = 1000000.f;

		private:
			Edge(Node *n1, Node *n2) : node1(n1), node2(n2) {
				node1 = n1;
				node2 = n2;
			}

			friend class Graph;
		};

		Graph() {}

		void select(Node *n, bool selected) {
			n->selected = selected;
			if (selected) {
				selected_nodes.insert(n);
			} else {
				selected_nodes.erase(n);
			}
		}

		// Ensures an edge between the given nodes exists.
		// Returns a new edge, or an existing equivalent one.
		// Returns null if either node is not part of this graph.
		Edge * addEdge(Node *n1, Node *n2) {
			if (nodes.find(n1) != nodes.end() && nodes.find(n2) != nodes.end()) {
				for (Edge *e : n1->getEdges()) {
					if (e->other(n1) == n2) return e;
				}
				Edge *e = new Edge(n1, n2);
				n1->addEdge(e);
				n2->addEdge(e);
				edges.insert(e);
				return e;
			}
			return nullptr;
		}

		Node * addNode(initial3d::vec3f pos, float ele = 0, float sharp = 0) {
			Node *n = new Node(pos, ele, sharp);
			nodes.insert(n);
			return n;
		}

		bool deleteEdge(Edge *e) {
			auto it = edges.find(e);
			if (it != edges.end()) {
				e->getNode1()->removeEdge(e);
				e->getNode2()->removeEdge(e);
				edges.erase(it);
				delete e;
				return true;
			}
			return false;
		}
			
		bool deleteNode(Node *n) {
			auto it = nodes.find(n);
			if (it != nodes.end()) {
				selected_nodes.erase(n);
				std::unordered_set<Edge *> edges0 = n->getEdges();
				for (Edge *e : edges0) {
					deleteEdge(e);
				}
				nodes.erase(it);
				return true;
			}
			return false;
		}


		const std::unordered_set<Node *> & getNodes() const {
			return nodes;
		}

		const std::unordered_set<Edge *> & getEdges() const {
			return edges;
		}

		const std::unordered_set<Node *> & getSelectedNodes() const {
			return selected_nodes;
		}

		void clearSelection() {
			for (Node *n : selected_nodes) {
				n->selected = false;
			}
			selected_nodes.clear();
		}

		// attempt some number of layout steps.
		// stops when average speed drops below threshold.
		// returns number of steps actually taken.
		int doLayout(int steps, const std::unordered_set<Node *> &active_nodes);

	private:
		std::unordered_set<Node *> nodes;
		std::unordered_set<Edge *> edges;

		std::unordered_set<Node *> selected_nodes;

		// for layout
		float timestep = 0.0001;

	};
}