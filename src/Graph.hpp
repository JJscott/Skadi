#pragma once

#include <cstdint>
#include <vector>


#include "Initial3D.hpp"


namespace skadi {

	class Node {
	public:
		Node(initial3d::vec3f pos) : position(pos), sharpness(0) {  }
		Node(initial3d::vec3f pos, float sharp) : position(pos), sharpness(sharp) {  }

		
		inline initial3d::vec3f getPosition() { return position; }
		inline void setPosition(initial3d::vec3f v) { position = v; }

		inline float getSharpness() { return sharpness; }
		inline void setSharpness(float s) { sharpness = s; }

	private:
		initial3d::vec3f position; //values of [0,1]
		float sharpness;

	};

	class Edge {
	public:
		Edge(Node *n1, Node *n2) : node1(n1), node2(n2) {}

		inline Node * other(Node *n) {
			return reinterpret_cast<Node *>(uintptr_t(node1) ^ uintptr_t(node2) ^ uintptr_t(n)); //Ben TODO check this
		}

		inline Node * getNode1() { return node1; }
		inline Node * getNode2() { return node2; }

	private:
		Node *node1;
		Node *node2;
	};
}