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

#include "Initial3D.hpp"
#include "Graph.hpp"


// : note :
//
// All absolute positions are in a 0.0 - 1.0 3d cube

namespace skadi {

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

			int squarity() const {
				return !((y - x) & ((gcdpow2(x) << 1u) - 1u));
			}

			void parents(index *p) const {
				int r = gcdpow2(x, y);
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

			int recursion_height() const {
				return (bit_scan_forward((1u << 31) | x | y) << 1u) + squarity();
			}
		};

		struct IndexHasher {
			std::size_t operator()(const index& k) const {
				using std::size_t;
				using std::hash;

				return ((hash<int>()(k.x) ^ (hash<int>()(k.y) << 1)) >> 1);
			}
		};


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
			return indices;
		};

	public:

		static std::vector<float> ridgeToHeightmap(const std::vector<Edge *> &edges, int size) {

			// Initialization
			//
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
				return unsigned(size * i.x + i.y);
			};

			auto distanceBetween = [&](const index &a, const index &b) -> float {
				float fsize = float(size - 1);
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
			auto compileParents = [&](const index &center, const std::vector<index> &parents) -> void {
				int centerIndex = getIdx(center);
				cellParents[centerIndex] = parents;
			};

			auto midpointDisplacement = [&](const index &center, const std::vector<index> &parents) -> void {
				int centerIndex = getIdx(center);

				// std::cout << "------ MD -------" << std::endl;
				// std::cout << "Elevation known " << std::boolalpha << elevationKnown[centerIndex] << std::endl;
				// std::cout << "[" << center.x << "," << center.y << "] = " << elevation[centerIndex] << " => ";
				// for (index p : parents) {
					// int parIndex = getIdx(p);
					// std::cout << " (" << p.x << "," << p.y << ") = " << elevation[parIndex];
				// }
				// std::cout << std::endl;

				if (!elevationKnown[centerIndex] && !parents.empty() ) {
					float e = 0;
					for (index par : parents) {
						int parIndex = getIdx(par);
						float etemp = elevationEstimate(
							elevation[ parIndex ],
							1,
							distanceBetween(center, par));
						// std::cout << " (" << par.x << "," << par.y << ") => " << etemp << std::endl;

						e += etemp;
					}
					e /= parents.size();
					// std::cout << "Final Size = "  << e << std::endl;
					elevation[centerIndex] = e;
					elevationKnown[centerIndex] = true;
				}

				// std::cout << " -> Press any key to continue . . . " << std::endl;
				// std::cin.get();
			};

			auto printParents = [&](const index &center, const std::vector<index> &parents) -> void {
				std::cout << "[" << center.x << "," << center.y << "] =>";
				for (index p : parents) {
					std::cout << " (" << p.x << "," << p.y << ")";
				}
				std::cout << std::endl;
			};


			// Record edge sparse data
			//
			auto addConstraint = [&] (index i, float e, float s) { //TODO check for duplicity
				int idx = getIdx(i);
				elevation[idx] = e;
				elevationKnown[idx] = true;
				// sharpness[idx] = s;
				constraints.push_back(i);
			};

			auto constrainEdge = [&](Edge *e) -> void {
				initial3d::vec3f v1 = e->getNode1()->getPosition();
				initial3d::vec3f v2 = e->getNode2()->getPosition();

				std::vector<index> indices = getLine(
					int(v1.x() * size), int(v2.x() * size),
					int(v1.z() * size), int(v2.z() * size));

				int i1 = 0;
				int i2 = indices.size()-1;

				addConstraint(indices[i1], v1.y(), 0);
				addConstraint(indices[i2], v2.y(), 0);

				std::function<void(int, int, float, float)> recursiveMD;
				recursiveMD = [&](int i1, int i2, float e1, float e2) -> void {
					if (abs(i1-i2) <= 1) return;
					int center = (i1 + i2) / 2;
					float centerElevation = (e1 + e2) / 2; //This can be changed to include random variance

					addConstraint(indices[center], centerElevation, 0);

					recursiveMD(i1, center, e1, centerElevation);
					recursiveMD(center, i2, centerElevation, e2);
				};

				recursiveMD(i1, i2, v1.y(), v2.y());
			};


			//Preprocess
			//
			diamondSquare(compileParents, size);
			// diamondSquare(printParents, size);


			// MBDU
			//
			std::unordered_map<index, std::vector<index>, IndexHasher> children;
			std::queue<index> fq;

			//std::cout << "MDBU" << std::endl

			for (Edge *e : edges) {
				constrainEdge(e);
			}

			for (index i : constraints) {
				fq.push(i);
			}

			while (!fq.empty()) {
				// std::cout << "Starting loop" << std::endl;
				// std::cout << "queue size : " << fq.size() << std::endl;

				while (!fq.empty()) {

					index cell = fq.front();
					unsigned cellIndex = getIdx(cell);
					fq.pop();

					// std::cout << "Popped index  :: " << cell.x << "," << cell.y << std::endl;
					// std::cout << "queue size : " << fq.size() << std::endl;

					// For each parent, add this cell to their list of children
					//
					// std::cout << "Set of parents {" << std::endl;
					for (index par : cellParents[cellIndex]) {

						int parIndex = getIdx(par);
						if (!elevationKnown[parIndex]) {
							if (children.find(par) != children.end()) {
								children[par] = std::vector<index>();
							}
							children[par].push_back(cell);
							// std::cout << "(" << par.x << "," << par.y << ")";
						}
					}
					// std::cout << "}" << std::endl;
				}

				// For each parent, child set, pair, estimate the elevation of the parent
				//
				// std::cout << std::endl << "CALCULATING HEIGHTMAP " << std::endl;
				for (auto it = children.begin(); it != children.end(); it++) {
					index par = it->first;
					unsigned parIndex = getIdx(par);
					elevation[parIndex] = 0;

					// std::cout << "Parent :: (" << par.x << "," << par.y << ")" << std::endl;
					
					// Calculating sum of estimated elevation
					//
					for (index cell : it->second) {
						unsigned cellIndex = getIdx(cell);
						float e = elevationEstimate(
							elevation[ cellIndex ],
							// interpolationValue[ cellIndex ],
							6,
							distanceBetween(cell, par));

						elevation[parIndex] += e;

						// std::cout << "Child : (" << cell.x << "," << cell.y << ") " << elevation[ cellIndex ] << " => " << e << std::endl;
					}

					// std::cout << "Parent :: elevation " << elevation[parIndex];

					// Divide estimated sum by n
					//
					elevation[parIndex] /= it->second.size();

					// std::cout << " | Final = " << elevation[parIndex] << std::endl << std::endl;

					elevationKnown[parIndex] = true;
					fq.push(par);
				}
				children.clear();
				// std::cout << " -> Press any key to continue . . . " << std::endl;
				// std::cin.get();
			}

			// std::vector<index> parToEval;

			// while (!fq.empty()) {
			// 	while (!fq.empty()) {
			// 		index cell = fq.front();
			// 		unsigned cellIndex = getIdx(cell);
			// 		fq.pop();

			// 		// For each parent, add this cell to their list of children
			// 		//
			// 		for (index par : cellParents[cellIndex]) {

			// 			if (children.find(par) != children.end()) {
			// 				children[par] = std::vector<index>();
			// 			}
			// 			children[par].push_back(cell);
			// 			parToEval.push_back(par);
			// 		}
			// 	}

			// 	// For each parent, child set, pair, estimate the elevation of the parent
			// 	//
			// 	for (index par : parToEval) {
			// 		unsigned parIndex = getIdx(par);
			// 		elevation[parIndex] = 0;
			// 		// Calculating sum of estimated elevation
			// 		//
			// 		for (index cell : children[par]) {
			// 			unsigned cellIndex = getIdx(cell);
			// 			float e = elevationEstimate(
			// 				elevation[ cellIndex ],
			// 				6,
			// 				distanceBetween(cell, par));

			// 			elevation[parIndex] += e;
			// 		}

			// 		// Divide estimated sum by n
			// 		//
			// 		elevation[parIndex] /= children[par].size();
			// 		elevationKnown[parIndex] = true;
			// 		fq.push(par);
			// 	}
			// 	parToEval.clear();
			// }



			// Top Down Midpoint Displacement
			//
			//std::cout << "MD" << std::endl;
			diamondSquare(midpointDisplacement, size);

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
	};
}
