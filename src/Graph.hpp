#pragma once


#include <algorithm>
#include <cstdint>
#include <unordered_set>
#include <vector>


#include "Initial3D.hpp"


namespace skadi {

	class Graph {
	public:

		class Node;
		class Edge;

		class Node {
		public:
			Node(initial3d::vec3f pos) : position(pos), sharpness(0) {  }
			Node(initial3d::vec3f pos, float sharp) : position(pos), sharpness(sharp) {  }

			inline const std::unordered_set<Edge *> & getEdges() { return edges; }
			inline void addEdge(Edge *e) { edges.emplace(e); }
			inline void removeEdge(Edge *e) { edges.erase(e); }
			inline bool containsEdge(Edge *e) { return edges.find(e) != edges.end(); }

			initial3d::vec3f position; //values of [0,1]
			float sharpness;
			bool selected;

		private:
			std::unordered_set<Edge *> edges;

			friend class Graph;
		};

		class Edge {
		public:

			inline Node * other(Node *n) {
				return reinterpret_cast<Node *>(uintptr_t(node1) ^ uintptr_t(node2) ^ uintptr_t(n)); //Ben TODO check this
			}

			inline Node * getNode1() { return node1; }
			inline Node * getNode2() { return node2; }

		private:
			Edge(Node *n1, Node *n2) : node1(n1), node2(n2) {
				node1 = n1;
				node2 = n2;
			}

			Node *node1;
			Node *node2;

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


		const std::vector<Node *> & getNodes() {
			return nodes;
		}

		const std::vector<Edge *> & getEdges() {
			return edges;
		}

	private:
		std::vector<Node *> nodes; //would be good to change this to a quad tree?
		std::vector<Edge *> edges;
	};
}