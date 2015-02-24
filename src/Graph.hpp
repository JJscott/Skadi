#pragma once


#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>


#include "Initial3D.hpp"
#include "Float3.hpp"


namespace skadi {

	class Graph {
	public:

		class Node;
		class Edge;

		class Node {
		public:
			Node(initial3d::float3 pos) : position(pos) {  }
			Node(initial3d::float3 pos, float sharp) : position(pos), sharpness(sharp) {  }

			const std::unordered_set<Edge *> & getEdges() const { return edges; }
			void addEdge(Edge *e) { edges.emplace(e); }
			void removeEdge(Edge *e) { edges.erase(e); }
			bool containsEdge(Edge *e) const { return edges.find(e) != edges.end(); }

			initial3d::float3 position; //values of [0,1]

			// for layout
			initial3d::float3 velocity;
			float mass = 1.f;
			float charge = 8.f;

			// TODO elevation etc

			float sharpness = 0.f;
			bool selected = false;
			
			// is the node locked in place
			bool fixed = false;

		private:
			std::unordered_set<Edge *> edges;

			friend class Graph;
		};

		class Edge {
		public:

			Node * other(Node *n) const {
				assert(n == node1 || n == node2);
				return reinterpret_cast<Node *>(uintptr_t(node1) ^ uintptr_t(node2) ^ uintptr_t(n)); //Ben TODO check this
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


		//creates an edges for the given nodes if they are recorded in this graph
		//Returns true if edge creation was successful
		Edge * addEdge(Node *n1, Node *n2) {
			if (std::find(nodes.begin(), nodes.end(), n1) != nodes.end() &&
				std::find(nodes.begin(), nodes.end(), n2) != nodes.end()) {
				for (Edge *e : n1->getEdges()) {
					if (n2->containsEdge(e))
						return nullptr;
				}
				Edge *e = new Edge(n1, n2);
				n1->addEdge(e);
				n2->addEdge(e);
				edges.push_back(e);
				return e;
			}
			return nullptr;
		}

		Node * addNode(initial3d::vec3f pos) {
			Node *n = new Node(pos);
			nodes.push_back(n);
			return n;
		}

		bool deleteEdge(Edge *e) {
			auto it = std::find(edges.begin(), edges.end(), e);
			if (it != edges.end()) {
				e->getNode1()->removeEdge(e);
				e->getNode2()->removeEdge(e);
				edges.erase(it);
				delete e;
				return true;
			}
			return false;
		}
			
		bool removeNode(Node *n) {
			auto it = std::find(nodes.begin(), nodes.end(), n);
			if (it != nodes.end()) {
				nodes.erase(it);
				return true;
			}
			return false;
		}


		const std::vector<Node *> & getNodes() const {
			return nodes;
		}

		const std::vector<Edge *> & getEdges() const {
			return edges;
		}

		// attempt some number of layout steps.
		// stops when average speed drops below threshold.
		// returns number of steps actually taken.
		int doLayout(int steps, const std::vector<Node *> &active_nodes);

	private:
		std::vector<Node *> nodes;
		std::vector<Edge *> edges;

		// for layout
		float timestep = 0.0001;

	};
}