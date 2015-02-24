
#include <unordered_set>

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

		class bh_node {
		private:
			aabb m_bound;
			float3 m_coc;
			bh_node *m_children[4];
			set_t m_values;
			size_t m_count = 0;
			float m_charge = 0;
			bool m_isleaf = true;

			inline unsigned childID(const float3 &p) const {
				return 0x3 & _mm_movemask_ps((p >= m_bound.center()).data());
			}

			// mask is true where cid bit is _not_ set
			inline __m128 childInvMask(unsigned cid) const {
				__m128i m = _mm_set1_epi32(cid);
				m = _mm_and_si128(m, _mm_set_epi32(0, 4, 2, 1));
				m = _mm_cmpeq_epi32(_mm_setzero_si128(), m);
				return _mm_castsi128_ps(m);
			}

			inline aabb childBound(unsigned cid) const {
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

			inline void unleafify();

		public:
			inline bh_node(const aabb &a_) : m_bound(a_), m_coc(a_.center()) {
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

			inline aabb bound() const {
				return m_bound;
			}

			inline size_t count() const {
				return m_count;
			}

			inline bool insert(Graph::Node *n, bool reinsert = false) {
				if (m_isleaf && m_count < 8) {
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

			inline float3 force(Graph::Node *n0) const {

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

			inline ~bh_node() {
				for (bh_node **pn = m_children + 4; pn --> m_children; ) {
					if (*pn) delete *pn;
				}
			}

		};

		bh_node *m_root = nullptr;

		// kill the z dimension of an aabb so this actually functions as a quadtree
		static inline aabb sanitize(const aabb &a) {
			float3 c = a.center();
			float3 h = a.halfsize();
			return aabb(float3(c.x(), c.y(), 0), float3(h.x(), h.y(), 0));
		}

		inline void destroy() {
			if (m_root) delete m_root;
			m_root = nullptr;
		}

	public:
		inline bh_tree() { }

		inline bh_tree(const aabb &rootbb) {
			m_root = new bh_node(sanitize(rootbb));
		}

		inline bh_tree(const bh_tree &other) {
			destroy();
			if (other.m_root) {
				m_root = new bh_node(*other.m_root);
			}
		}

		inline bh_tree(bh_tree &&other) {
			m_root = other.m_root;
			other.m_root = nullptr;
		}

		inline bh_tree & operator=(const bh_tree &other) {
			destroy();
			if (other.m_root) {
				m_root = new bh_node(*other.m_root);
			}
			return *this;
		}

		inline bh_tree & operator=(bh_tree &&other) {
			destroy();
			m_root = other.m_root;
			other.m_root = nullptr;
			return *this;
		}

		inline bool insert(Graph::Node *n) {
			if (!m_root) m_root = new bh_node(aabb(float3(0), float3(1, 1, 0)));
			if (m_root->bound().contains(n->position)) {
				return m_root->insert(n);
			} else {
				assert(false && "not implemented yet");
				return false;
			}
		}

		inline float3 force(Graph::Node *n0) {
			if (!m_root) return float3(0);
			return m_root->force(n0);
		}

		inline ~bh_tree() {
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


}


namespace skadi {


	int Graph::doLayout(int steps, const std::vector<Node *> &active_nodes) {


		return 0;
	}


}
