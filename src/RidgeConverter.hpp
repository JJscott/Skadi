#pragma once

#include <cstdint>
#include <cassert>

#if defined(_MSC_VER)
// Visual Studio / MSVC
#include <intrin.h>
namespace skadi {
	inline uint32_t bit_scan_forward(uint32_t x) {
		assert(x);
		unsigned long i;
		_BitScanForward(&i, x);
		return i;
	}

	inline uint32_t bit_scan_reverse(uint32_t x) {
		assert(x);
		unsigned long i;
		_BitScanReverse(&i, x);
		return i;
	}
}
#elif defined(__GNUC__)
// GCC, Clang
namespace skadi {
	inline uint32_t bit_scan_forward(uint32_t x) {
		assert(x);
		return __builtin_ctz(x);
	}

	inline uint32_t bit_scan_reverse(uint32_t x) {
		assert(x);
		return 31u - __builtin_clz(x);
	}
}
#endif


#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <queue>
#include <vector>
#include <random>
#include <iostream>
#include <iomanip>
#include <utility>

#include "Initial3D.hpp"
#include "Graph.hpp"

// i reverted these to commit 7cd00b0a cause it almost unbreaks things
// also, why the fuck did you use defines?
#define SHARP -0.5
#define BU_SHARP 4.0

// : note :
//
// All absolute positions are in a 0.0 - 1.0 3d cube

namespace skadi {

	inline void really_assert(bool b) {
		if (!b) throw "nope!";
	}

	inline uint32_t gcdpow2(uint32_t x0) {
		// ensure highest bit is set in argument for sane result
		return 1u << bit_scan_forward((1u << 31) | x0);
	}

	template <typename ...TR>
	inline uint32_t gcdpow2(uint32_t x0, uint32_t x1, TR ...tr) {
		return gcdpow2(x0 | x1, tr...);
	}

	inline void println() {
		std::cout << std::endl;
	}

	template <typename T1, typename ...TR>
	inline void println(const T1 &t1, const TR &...tr) {
		std::cout << t1;
		println(tr...);
	}


	template <typename RandItT, typename ValT, typename PredT>
	inline void push_heap_weak_impl(RandItT first, RandItT last, ValT &&val, PredT comp) {
		// TODO this isnt finished yet
		// push *last
		auto hole = last - first;
		for (auto i = hole; i > 0; ) {
			// go to parent
			i = (i - 1) / 2;
			auto c = comp(val, *(first + i));
			if (c > 0) {
				// ordered-after: stop
				break;
			} else {
				// ordered-before or not ordered: move hole and keep searching parents
				*(first + hole) = std::move(*(first + i));
				hole = i;
			}
		}
		// move value to final hole
		*(first + hole) = std::move(val);
	}

	template <typename RandItT, typename PredT>
	inline void push_heap_weak(RandItT first, RandItT last, PredT comp) {
		// push *(last - 1)
		if (last - first > 1) {
			--last;
			auto val = std::move(*last);
			push_heap_weak_impl(first, last, val, comp);
		}
	}


	template <typename RandItT, typename PredT>
	inline void pop_heap_weak(RandItT first, RandItT last, PredT comp) {
		auto comp2 = [&](const auto &a, const auto &b) {
			return comp(a, b) > 0;
		};
		std::pop_heap(first, last, comp2);
		assert(std::is_heap(first, last - 1, comp2));
	}



	template <typename T, typename CompareT>
	class weak_priority_queue {
		std::vector<T> m_data;
		CompareT m_comp;

	public:
		weak_priority_queue() { }

		bool empty() const {
			return m_data.empty();
		}

		const T & top() const {
			return m_data.front();
		}

		void pop() {
			pop_heap_weak(m_data.begin(), m_data.end(), m_comp);
			m_data.pop_back();
		}

		void push(T t) {
			m_data.emplace_back(std::move(t));
			push_heap_weak(m_data.begin(), m_data.end(), m_comp);
		}
	};


	class RidgeConverter {
	private:

		struct index {
			int x;
			int y;

			index() : x(0), y(0) { }

			index(int _x, int _y) : x(_x), y(_y) {  }

			// TODO josh: why do you have this?
			index(float _x, float _y) : x(int(_x)), y(int(_y)) {  }

			bool operator==(const index &other) const {
				return (x == other.x) && (y == other.y);
			}

			unsigned squarity() const {
				return !((y - x) & ((gcdpow2(x) << 1u) - 1u));
			}

			void parents(index *p) const {
				int r = parent_radius();
				int smask = squarity() ? -1 : 0;
				p[0].x = x + r;
				p[0].y = y + (r & smask);
				p[1].x = x - (r & smask);
				p[1].y = y + r;
				p[2].x = x - r;
				p[2].y = y - (r & smask);
				p[3].x = x + (r & smask);
				p[3].y = y - r;
			}

			unsigned parent_radius() const {
				return gcdpow2(x, y);
			}

			unsigned recursion_height() const {
				return (bit_scan_forward((1u << 31) | x | y) << 1u) + squarity();
			}

			bool parent_of(const index &p) const {
				if (*this == p) return false;
				// test feasibility
				// TODO is this correct?
				if (this->parent_radius() < p.parent_radius()) return false;
				index a[4];
				p.parents(a);
				// test direct parents
				for (int i = 0; i < 4; i++) {
					if (*this == a[i]) return true;
				}
				// test recursive parents
				for (int i = 0; i < 4; i++) {
					if (parent_of(a[i])) return true;
				}
				return false;
			}

		};

		struct IndexHasher {
			std::size_t operator()(const index& k) const {
				using std::size_t;
				using std::hash;

				return ((hash<int>()(k.x) ^ (hash<int>()(k.y) << 1)) >> 1);
			}
		};

		// http://stackoverflow.com/questions/10060046/drawing-lines-with-bresenhams-line-algorithm
		static std::vector<index> bhm_line(int x1, int y1, int x2, int y2) {
			std::vector<index> indices;
			int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;
			dx=x2-x1;
			dy=y2-y1;
			dx1=abs(dx);
			dy1=abs(dy);
			px=2*dy1-dx1;
			py=2*dx1-dy1;
			if(dy1<=dx1) {
				if(dx>=0) {
					x=x1;
					y=y1;
					xe=x2;
				} else {
					x=x2;
					y=y2;
					xe=x1;
				}
				//putpixel(x,y,c);
				indices.emplace_back(x, y);
				for(i=0;x<xe;i++) {
					x=x+1;
					if(px<0) {
						px=px+2*dy1;
					} else {
						if((dx<0 && dy<0) || (dx>0 && dy>0)) {
							y=y+1;
						} else {
							y=y-1;
						}
						px=px+2*(dy1-dx1);
					}
					//putpixel(x,y,c);
					indices.emplace_back(x, y);
				}
			} else {
				if(dy>=0) {
					x=x1;
					y=y1;
					ye=y2;
				} else {
					x=x2;
					y=y2;
					ye=y1;
				}
				//putpixel(x,y,c);
				indices.emplace_back(x, y);
				for(i=0;y<ye;i++) {
					y=y+1;
					if(py<=0) {
						py=py+2*dx1;
					} else {
						if((dx<0 && dy<0) || (dx>0 && dy>0)) {
							x=x+1;
						} else {
							x=x-1;
						}
						py=py+2*(dx1-dy1);
					}
					//putpixel(x,y,c);
					indices.emplace_back(x, y);
				}
			}

			assert(!indices.empty());

			if (!(indices[0] == index(x1, y1))) {
				// make sure the indices start at x1,y1
				std::reverse(indices.begin(), indices.end());
			}

			return indices;
		}

		static std::vector<index> getLine(int x1, int x2, int y1, int y2) {
			std::vector<index> indices;
			const bool steep = (abs(y2 - y1) > abs(x2 - x1));
			const bool reverse = (x1 > x2);
			if(steep) {
				std::swap(x1, y1);
				std::swap(x2, y2);
			}

			if(reverse)	{
				std::swap(x1, x2);
				std::swap(y1, y2);
			}

			const float dx = x2 - x1;
			const float dy = abs(y2 - y1);

			float error = dx / 2.0f;
			const int ystep = (y1 < y2) ? 1 : -1;
			int y = int(y1);

			const int maxX = int(x2);

			for(int x = int(x1); x < maxX; x++) {
				if (steep) indices.push_back(index(y, x));
				else indices.push_back(index(x, y));
				error -= dy;
				if(error < 0) {
					y += ystep;
					error += dx;
				}
			}
			if (reverse)
				std::reverse(indices.begin(), indices.end());

			assert(!indices.empty());

			return indices;
		};

	public:

		static std::vector<float> ridgeToHeightmap(const std::vector<Graph::Edge *> &edges, int size) {

			// Initialization
			//
			gecom::log("Heightmap") << "Initializing...";

			std::vector<float> elevation;
			// std::vector<float> sharpness;
			std::vector<bool> elevationKnown;
			std::vector<index> constraints;
			std::vector<std::vector<index>> cellParents;

			for (int y = 0; y < size; y++) {
				for (int x = 0; x < size; x++) {
					elevation.push_back(0);
					// sharpness.push_back(0);
					elevationKnown.push_back(false);
					cellParents.push_back(std::vector<index>());
				}
			}

			float maxDistance = sqrt(2);

			// std::default_random_engine generator;
			// std::uniform_real_distribution<float> (0.0, 1.0);

			// float randTrans = 0; // random value translation
			// float randScale = 0; // random value scale



			// Define index utility functions
			//

			// auto idx = [&](int x, int y) -> unsigned {
			// 	return unsigned(size * y + x);
			// };

			auto getIdx = [&](index i) -> unsigned {
				int x = std::max(std::min(i.x, size-1), 0);
				int y = std::max(std::min(i.y, size-1), 0);
				return unsigned(size * i.y + i.x);
			};

			auto distanceBetween = [&](const index &a, const index &b) -> float {
				static const float fsize = float(hypot(size, size));
				return hypot((a.x - b.x) / fsize, (a.y - b.y) / fsize);
			};

			// Define index and estimation functions
			//
			auto interpValue = [](float i)-> float {
				return (i >= 0) ? 1 : -1;
			};

			auto elevationEstimate = [&](float e, float i, float d) -> float {
				return e * (1 - interpValue(i) * (1- pow(1-d/maxDistance, abs(i)) ));
			};

			// Define diamond square compatible functions
			//
			auto midpointDisplacement = [&](const index &center, const std::vector<index> &parents) -> void {
				int centerIndex = getIdx(center);

				if (!elevationKnown[centerIndex] && !parents.empty() ) {
					float e = 0;
					for (index par : parents) {
						int parIndex = getIdx(par);
						float etemp = elevationEstimate(
							elevation[ parIndex ],
							SHARP,
							distanceBetween(center, par));
						e += etemp;
					}
					e /= parents.size();
					elevation[centerIndex] = e;
					elevationKnown[centerIndex] = true;
				}

				// std::cout << " -> Press any key to continue . . . " << std::endl;
				// std::cin.get();
			};

			// DEBUG
			//
			// auto printParents = [&](const index &center, const std::vector<index> &parents) -> void {
			// 	std::cout << "[" << center.x << "," << center.y << "] =>";
			// 	for (index p : parents) {
			// 		std::cout << " (" << p.x << "," << p.y << ")";
			// 	}
			// 	std::cout << std::endl;
			// };
			// diamondSquare(printParents, size);


			// Record edge sparse data
			//
			auto addConstraint = [&] (index i, float e, float s) { //TODO check for duplicity
				int idx = getIdx(i);
				elevation[idx] = e;
				elevationKnown[idx] = true;
				// sharpness[idx] = s;
				constraints.push_back(i);
			};

			auto constrainEdge = [&](Graph::Edge *e) -> void {
				initial3d::vec3f v1 = e->getNode1()->position;
				initial3d::vec3f v2 = e->getNode2()->position;

				float ne1 = e->getNode1()->elevation;
				float ne2 = e->getNode2()->elevation;

				float ns1 = e->getNode1()->sharpness;
				float ns2 = e->getNode2()->sharpness;

				//std::vector<index> indices = getLine(
				//	int(v1.x() * size), int(v2.x() * size),
				//	int(v1.y() * size), int(v2.y() * size)
				//);

				// note: different arg order
				std::vector<index> indices = bhm_line(v1.x() * size, v1.y() * size, v2.x() * size, v2.y() * size);

				int i1 = 0;
				int i2 = indices.size()-1;

				addConstraint(indices[i1], ne1, ns1);
				addConstraint(indices[i2], ne2, ns2);

				//std::cout << "Edge => [" << v1 << "," << v2 << "] <=> " << ne1 << "," << ne2 << std::endl;

				std::function<void(int, int, float, float)> recursiveMD;
				recursiveMD = [&](int i1, int i2, float e1, float e2) -> void {
					if (abs(i1-i2) <= 1) return;
					int center = (i1 + i2) / 2;
					float centerElevation = (e1 + e2) / 2; //This can be changed to include random variance

					addConstraint(indices[center], centerElevation, 0);

					recursiveMD(i1, center, e1, centerElevation);
					recursiveMD(center, i2, centerElevation, e2);
				};

				recursiveMD(i1, i2, ne1, ne2);
			};


			//Preprocess
			//
			auto compileParents = [&](const index &center, const std::vector<index> &parents) -> void {
				int centerIndex = getIdx(center);
				cellParents[centerIndex] = parents;
			};
			diamondSquare(compileParents, size);


			std::function<void(const index &, const std::vector<index> &)> verifyParents;
			verifyParents = [&](const index &a, const std::vector<index> &p) {

				really_assert(!a.parent_of(a));

				for (const index &b : p) {
					really_assert(b.parent_of(a));
					really_assert(!a.parent_of(b));
					verifyParents(a, cellParents[getIdx(b)]);
				}


			};


			struct compare_indices {
				// true if a should be popped after b
				bool operator()(const index &a, const index &b) {
					//if (a == b) return false;
					bool ba = a.parent_of(b);
					bool bb = b.parent_of(a);
					assert(!(ba && bb));
					return bb ? -1 : ba ? 1 : 0;
				}
			};

			

			// With black magic
			//
			std::unordered_map<index, std::unordered_set<index, IndexHasher>, IndexHasher> children;
			weak_priority_queue<index, compare_indices> fq;

			gecom::log("Heightmap") << "Constrainining edges..";
			for (Graph::Edge *e : edges) {
				constrainEdge(e);
			}

			gecom::log("Heightmap") << "Pushing constraints...";
			for (index i : constraints) {
				fq.push(i);
			}

			gecom::log("Heightmap") << "Invoking darker-than-black magic...";


			while (!fq.empty()) {
				
				
				index cell = fq.top();
				fq.pop();

				// add this cell as a child of its unknown parents, and add those parents to the queue
				if (!(cell == index(0, 0))) {
					index pArr[4];
					cell.parents(pArr);
					for (int i = 0; i < 4; i++) {
						index par = pArr[i];
						if (
							par.x >= 0 && par.x < size &&
							par.y >= 0 && par.y < size &&
							!elevationKnown[getIdx(par)]
						) {
							if (children[par].insert(cell).second) {
								// only add parent to queue if child not already recorded as such
								fq.push(par);
							}
						}
					}
				}

				// index is NOT a good name for the 2D grid position thingy
				unsigned cell_idx = getIdx(cell);
				
				// check for (and avoid) re-evaluation
				if (elevationKnown[cell_idx]) {
					continue;
				}

				// find known children of this cell
				auto it = children.find(cell);
				assert(it != children.end());
				assert(it->second.size() != 0);

				// estimate elevation for this cell from known children
				float et = 0;
				for (index child : it->second) {
					float e = elevationEstimate(
						elevation[getIdx(child)],
						// interpolationValue[ cellIndex ],
						BU_SHARP,
						distanceBetween(child, cell)
					);
					et += e;
				}

				// set new elevation for this cell
				elevation[cell_idx] = et / it->second.size();
				elevationKnown[cell_idx] = true;

				// remove unnecessary entries from children map
				children.erase(it);

			}


			// Top Down Midpoint Displacement
			//
			gecom::log("Heightmap") << "Midpoint displacement...";
			//std::cout << "MD" << std::endl;
			diamondSquare(midpointDisplacement, size);

			gecom::log("Heightmap") << "It worked?";

			return elevation;
		}

		static void diamondSquare(std::function<void(const index &, const std::vector<index> &)> func, int size) {
			//using index = std::pair<int, int>;

			//note: this algorithm is based on the assumption of size = (2^n)+1
			//

			int upperLimit = size - 1;
			int stepsize = upperLimit / 2;
			while (stepsize > 0) {

				//find midpoints of all the squares
				//set the parents of the midpoints to be the diagonals from the centers
				//
				for (int y = stepsize; y < upperLimit; y += 2 * stepsize) {
					for (int x = stepsize; x < upperLimit; x += 2 * stepsize) {
						index centerIndex(x, y);

						std::vector<index> parents;
						parents.push_back(index(x - stepsize, y - stepsize));
						parents.push_back(index(x + stepsize, y - stepsize));
						parents.push_back(index(x - stepsize, y + stepsize));
						parents.push_back(index(x + stepsize, y + stepsize));

						func(centerIndex, parents);
					}
				}


				//find midpoints of edges
				//set parents of edges to be the adjacent midpoints
				//
				for (int v = 0; v <= upperLimit; v += 2 * stepsize) {
					for (int u = stepsize; u < upperLimit; u += 2 * stepsize) {


						//  o--X--o--X--
						//  |  |  |  |  
						//  |--o--|--o--
						//  |  |  |  |  
						//  o--X--o--X--
						//  |  |  |  |

						//  rows u => x, v => y
						//
						int x = u, y = v;
						index centerIndex(x, y);
						std::vector<index> parents;

						index t(x, y - stepsize);
						index b(x, y + stepsize);
						index l(x - stepsize, y);
						index r(x + stepsize, y);

						if (t.y > 0) parents.push_back(t);
						if (b.y < upperLimit) parents.push_back(b);
						parents.push_back(l);
						parents.push_back(r);

						func(centerIndex, parents);

						//  o-----o--
						//  |  |  |  
						//  X--o--X--
						//  |  |  |  
						//  o-----o--
						//  |  |  |  
						//  X--o--X--
						//  |  |  |

						//  columns v => x, u => y
						//
						x = v; y = u;
						centerIndex = index(x, y);
						parents = std::vector<index>();

						t = index(x, y - stepsize);
						b = index(x, y + stepsize);
						r = index(x + stepsize, y);
						l = index(x - stepsize, y);

						if (l.x > 0) parents.push_back(l);
						if (r.x < upperLimit) parents.push_back(r);
						parents.push_back(t);
						parents.push_back(b);

						func(centerIndex, parents);
					}
				}

				//calculate step size and repeat until stepsize is 0
				stepsize /= 2;
			}
		}

		static void test_shit() {
			index a(32, 96);
			index b(24, 112);

			

			std::cout << a.parent_of(b) << std::endl;
			std::cout << b.parent_of(a) << std::endl;


		}

	};
}
