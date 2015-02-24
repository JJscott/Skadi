
#include <unordered_set>
#include <algorithm>
#include <iterator>
#include <cstring>

#include "Graph.hpp"

using namespace std;
using namespace initial3d;
using namespace skadi;

namespace {

	// Barnes-Hut quadtree for charge repulsion
	// http://www.cs.princeton.edu/courses/archive/fall03/cs126/assignments/barnes-hut.html
	class bh_tree {
	private:
		using set_t = unordered_set<Graph::Node *>;

		static const int max_leaf_elements = 8;

		class bh_node {
		private:
			aabb m_bound;
			float3 m_coc;
			bh_node *m_children[4];
			set_t m_values;
			size_t m_count = 0;
			float m_charge = 0;
			bool m_isleaf = true;

			unsigned childID(const float3 &p) const {
				return 0x3 & _mm_movemask_ps((p >= m_bound.center()).data());
			}

			// mask is true where cid bit is _not_ set
			__m128 childInvMask(unsigned cid) const {
				__m128i m = _mm_set1_epi32(cid);
				m = _mm_and_si128(m, _mm_set_epi32(0, 4, 2, 1));
				m = _mm_cmpeq_epi32(_mm_setzero_si128(), m);
				return _mm_castsi128_ps(m);
			}

			aabb childBound(unsigned cid) const {
				// positive / negative halfsizes
				__m128 h = m_bound.halfsize().data();
				__m128 g = _mm_sub_ps(_mm_setzero_ps(), h);

				// convert int bitmask to (opposite) sse mask
				__m128 n = childInvMask(cid);

				// vector to a corner of the current node's aabb
				float3 vr(_mm_or_ps(_mm_and_ps(n, g), _mm_andnot_ps(n, h)));
				const float3 c = m_bound.center();

				return aabb::fromPoints(c, c + vr);
			}

			void unleafify();
			void leafify();

			void dump(set_t &values) {
				// move values out of this node
				for (auto v : m_values) {
					values.insert(v);
				}
				// move values out of child nodes
				for (bh_node **pn = m_children + 4; pn --> m_children; ) {
					if (*pn) (*pn)->dump(values);
				}
				// safety
				m_values.clear();
				m_count = 0;
			}

		public:
			bh_node(const aabb &a_) : m_bound(a_), m_coc(a_.center()) {
				// clear child pointers
				std::memset(m_children, 0, 4 * sizeof(bh_node *));
			}

			bh_node(const bh_node &other) :
				m_bound(other.m_bound),
				m_coc(other.m_coc),
				m_values(other.m_values),
				m_count(other.m_count),
				m_charge(other.m_charge),
				m_isleaf(other.m_isleaf)
			{
				// clear child pointers
				std::memset(m_children, 0, 4 * sizeof(bh_node *));
				// clone children
				for (int i = 0; i < 4; i++) {
					bh_node *oc = other.m_children[i];
					if (oc) {
						m_children[i] = new bh_node(*oc);
					}
				}
			}

			bh_node & operator=(const bh_node &) = delete;

			aabb bound() const {
				return m_bound;
			}

			size_t count() const {
				return m_count;
			}

			bool insert(Graph::Node *n, bool reinsert = false) {
				if (m_isleaf && m_count < max_leaf_elements) {
					if (!m_values.insert(n).second) return false;
				} else {
					// not a leaf or should not be
					unleafify();
					unsigned cid = childID(n->position);
					// element contained in one child node (its a point) - create if necessary then insert
					bh_node *child = m_children[cid];
					if (!child) {
						child = new bh_node(childBound(cid));
						m_children[cid] = child;
					}
					if (!child->insert(n)) return false;
				}
				// allow re-inserting internally to skip accumulation
				if (reinsert) return true;
				m_count++;
				// update charge and centre-of-charge
				m_coc = (m_coc * m_charge + n->position * n->charge) / (m_charge + n->charge);
				m_charge += n->charge;
				return true;
			}

			void put_child(bh_node *child) {
				unsigned cid = childID(child->bound().center());
				assert(!m_children[cid]);
				m_children[cid] = child;
				m_count += child->count();
				// need to ensure the child is added properly
				unleafify();
				if (m_count <= max_leaf_elements) {
					// if this should actually be a leaf after all
					leafify();
				}
			}

			float3 force(Graph::Node *n0) const {

				// can we treat this node as one charge?
				// compare bound width to distance from node to centre-of-charge
				{
					// direction is away from coc
					float3 v = n0->position - m_coc;
					float id2 = 1.f / float3::dot(v, v);
					float s = m_bound.halfsize().x() + m_bound.halfsize().y();
					float q2 = s * s * id2;
					// note that this is the square of the ratio of interest
					// too much higher and it doesnt converge very well
					if (q2 < 0.5) {
						float k = min(id2 * n0->charge * m_charge, 100000.f);
						// shouldnt need to nan check
						return v.unit() * k;
					}
				}

				float3 f(0);

				// force from nodes in this node
				for (auto n1 : m_values) {
					if (n1 == n0) continue;
					// direction is away from other node
					float3 v = n0->position - n1->position;
					float id2 = 1.f / float3::dot(v, v);
					float k = min(id2 * n0->charge * n1->charge, 100000.f);
					float3 fc = v.unit() * k;
					if (fc.isnan()) {
						f += float3(0, 0.1, 0);
					} else {
						f += fc;
					}
				}

				// recurse
				for (bh_node * const *pn = m_children + 4; pn --> m_children; ) {
					if (*pn) {
						f += (*pn)->force(n0);
					}
				}

				return f;
			}

			~bh_node() {
				for (bh_node **pn = m_children + 4; pn --> m_children; ) {
					if (*pn) delete *pn;
				}
			}

		};

		bh_node *m_root = nullptr;

		// kill the z dimension of an aabb so this actually functions as a quadtree
		static aabb sanitize(const aabb &a) {
			float3 c = a.center();
			float3 h = a.halfsize();
			return aabb(float3(c.x(), c.y(), 0), float3(h.x(), h.y(), 0));
		}

		void destroy() {
			if (m_root) delete m_root;
			m_root = nullptr;
		}

	public:
		bh_tree() { }

		bh_tree(const aabb &rootbb) {
			m_root = new bh_node(sanitize(rootbb));
		}

		bh_tree(const bh_tree &other) {
			destroy();
			if (other.m_root) {
				m_root = new bh_node(*other.m_root);
			}
		}

		bh_tree(bh_tree &&other) {
			m_root = other.m_root;
			other.m_root = nullptr;
		}

		bh_tree & operator=(const bh_tree &other) {
			destroy();
			if (other.m_root) {
				m_root = new bh_node(*other.m_root);
			}
			return *this;
		}

		bh_tree & operator=(bh_tree &&other) {
			destroy();
			m_root = other.m_root;
			other.m_root = nullptr;
			return *this;
		}

		bool insert(Graph::Node *n) {
			if (!m_root) m_root = new bh_node(aabb(float3(0), float3(1, 1, 0)));
			if (m_root->bound().contains(n->position)) {
				return m_root->insert(n);
			} else {
				// make new root
				bh_node * const oldroot = m_root;
				const aabb a = m_root->bound();
				// vector from centre of current root to centre of new element
				const float3 vct = n->position - a.center();
				// vector from current root to corner nearest centre of new element
				float3 corner = a.halfsize();
				corner = float3::mixb(corner, -corner, vct < 0.f);
				// centre of new root
				const float3 newcentre = a.center() + corner;
				m_root = new bh_node(sanitize(aabb(newcentre, a.halfsize() * 2.f)));
				if (oldroot->count() > 0) {
					// only preserve old root if it had elements
					m_root->put_child(oldroot);
				} else {
					delete oldroot;
				}
				// re-attempt to add
				return insert(n);
			}
		}

		float3 force(Graph::Node *n0) {
			if (!m_root) return float3(0);
			return m_root->force(n0);
		}

		~bh_tree() {
			destroy();
		}

	};

	inline void bh_tree::bh_node::unleafify() {
		if (m_isleaf) {
			m_isleaf = false;
			set_t temp = move(m_values);
			for (auto n : temp) {
				insert(n, true);
			}
		}
	}

	inline void bh_tree::bh_node::leafify() {
		if (!m_isleaf) {
			m_isleaf = true;
			// dump values in child nodes into this one
			for (bh_node **pn = m_children + 4; pn --> m_children; ) {
				if (*pn) {
					(*pn)->dump(m_values);
					delete *pn;
					*pn = nullptr;
				}
			}
		}
	}

}


namespace skadi {


	int Graph::doLayout(int steps, const std::vector<Node *> &active_nodes) {

		// tree of nodes that will not move
		bh_tree bht0;
		
		// build tree of nodes that wont move (or otherwise change)
		// NOTE this is slow with VS debugger attached, even a release-mode build
		for (auto n : nodes) {
			if (n->fixed || find(active_nodes.begin(), active_nodes.end(), n) == active_nodes.end()) {
				// node fixed or not active
				bht0.insert(n);
			}
		}

		// nodes that will be moved
		vector<Node *> nodes0;
		copy_if(active_nodes.begin(), active_nodes.end(), back_inserter(nodes0), [](Node *n) { return !n->fixed; });

		// run steps
		for (int step = 0; step < steps; step++) {
			
			float speed_sum = 0.f;

			// add moving nodes to new tree
			bh_tree bht(bht0);
			for (auto n : nodes0) {
				bht.insert(n);
			}

			// calculate forces, accelerations, velocities; get average speed
#pragma omp parallel for reduction(+:speed_sum)
			for (int i = 0; i < nodes0.size(); i++) {
				Node *n0 = nodes0[i];

				// acting force
				float3 f;

				// charge repulsion from every node
				f += bht.force(n0);

				// spring contraction from connected nodes
				for (auto e : n0->getEdges()) {
					// other node
					Node *n1 = e->other(n0);
					// direction is towards other node
					float3 v = n1->position - n0->position;
					f += e->spring * v; // spring constant
				}

				// acceleration
				float3 a = f / n0->mass;

				// velocity
				n0->velocity += a * timestep;

				// damping
				n0->velocity *= 0.995;

				speed_sum += n0->velocity.mag();
			}

			// update positions
#pragma omp parallel for
			for (int i = 0; i < nodes0.size(); i++) {
				Node *n0 = nodes0[i];
				n0->position += n0->velocity * timestep;
			}

			// TODO tune threshold
			if (speed_sum / nodes0.size() < 2.f) return step;
		}

		return steps;
	}


}
