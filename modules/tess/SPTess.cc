/**
 Copyright (c) 2022 Roman Katuntsev <sbkarr@stappler.org>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 **/

#include "SPTess.h"
#include "SPTessTypes.h"
#include "SPLog.h"

namespace stappler::geom {

struct Tesselator::Data : ObjectAllocator {
	// potential root face edges (connected to right non-convex angle)
	Vec2 _bmax, _bmin;

	TessResult *_result = nullptr;
	EdgeDict* _edgeDict = nullptr;
	VertexPriorityQueue *_vertexQueue = nullptr;

	float _mathTolerance = std::numeric_limits<float>::epsilon();

	Winding _winding = Winding::NonZero;
	float _antialiasValue = 0.0f;
	uint32_t _nvertexes = 0;
	uint8_t _markValue = 0;

	VerboseFlag _verbose = VerboseFlag::General;

	Data(memory::pool_t *p);

	void computeInterior();
	void tessellateInterior();
	bool tessellateMonoRegion(HalfEdge *, uint8_t);
	void sweepVertex(VertexPriorityQueue &pq, EdgeDict &dict, Vertex *v);
	HalfEdge * processIntersect(Vertex *, const EdgeDictNode *, HalfEdge *, Vec2 &, IntersectionEvent ev);

	Edge *makeEdgeLoop(const Vec2 &origin);

	Vertex *makeVertex(HalfEdge *eOrig);
	Face *makeFace(HalfEdge *eOrig, Face *fNext);

	HalfEdge *pushVertex(HalfEdge *e, const Vec2 &, bool clockwise = false);
	HalfEdge *connectEdges(HalfEdge *eOrg, HalfEdge *eDst);

	Vertex *splitEdge(HalfEdge *, HalfEdge *eOrg2, const Vec2 &);

	HalfEdge *getFirstEdge(Vertex *org) const;
	void mergeVertexes(Vertex *org, Vertex *merge);
	HalfEdge * removeEdge(HalfEdge *);

	HalfEdge * removeDegenerateEdges(HalfEdge *, uint32_t *nedges);
};

Tesselator::~Tesselator() {
	if (_data) {
		auto pool = _data->_pool;
		_data->~Data();
		_data = nullptr;
		memory::pool::destroy(pool);
	}
}

bool Tesselator::init(memory::pool_t *pool) {
	auto p = memory::pool::create(pool);
	memory::pool::context ctx(p);

	_data = new (p) Data(p);
	return true;
}

void Tesselator::setAntialiasValue(float value) {
	_data->_antialiasValue = value;
}

float Tesselator::getAntialiasValue() const {
	return _data->_antialiasValue;
}

void Tesselator::setWindingRule(Winding winding) {
	_data->_winding = winding;
}

Winding Tesselator::getWindingRule() const {
	return _data->_winding;
}

void Tesselator::preallocate(uint32_t n) {
	_data->preallocateVertexes(n);
	_data->preallocateEdges(n);
}

Tesselator::Cursor Tesselator::beginContour(bool clockwise) {
	return Cursor{nullptr, clockwise};
}

bool Tesselator::pushVertex(Cursor &cursor, const Vec2 &vertex) {
	if (!cursor.closed) {
		if (!cursor.edge || !VertEq(cursor.edge->getDstVec(), vertex, _data->_mathTolerance)) {
			if (_data->_verbose != VerboseFlag::None) {
				std::cout << "Push: " << vertex << "\n";
			}

			cursor.edge = _data->pushVertex(cursor.edge, vertex, cursor.isClockwise);
			++ cursor.count;
			return true;
		}
	}

	return false;
}

bool Tesselator::closeContour(Cursor &cursor) {
	if (cursor.closed) {
		return false;
	}

	cursor.closed = true;

	cursor.edge = _data->removeDegenerateEdges(cursor.edge, &cursor.count);

	if (cursor.edge) {
		if (_data->_verbose != VerboseFlag::None) {
			std::cout << "Contour:\n";
			cursor.edge->foreachOnFace([&] (HalfEdge &e) {
				std::cout << _data->_verbose << "\t" << e << "\n";
			});
		}
		return true;
	} else {
		if (_data->_verbose != VerboseFlag::None) {
			std::cout << "Fail to add empty contour\n";
		}
	}
	_data->trimVertexes();
	return false;
}

bool Tesselator::prepare(TessResult &res) {
	_data->_result = &res;
	_data->_vertexOffset = res.nvertexes;
	_data->computeInterior();
	_data->tessellateInterior();
	_data->_result = nullptr;

	res.nvertexes += _data->_exportVertexes.size();
	res.nfaces += _data->_faceEdges.size();

	return false;
}

bool Tesselator::write(TessResult &res) {
	for (auto &it : _data->_exportVertexes) {
		if (it) {
			res.pushVertex(res.target, it->_queueIdx + _data->_vertexOffset, it->_origin, 1.0f);
		}
	}

	uint32_t triangle[3] = { 0 };
	auto mark = ++ _data->_markValue;
	for (auto &it : _data->_faceEdges) {
		if (it->_mark != mark && isWindingInside(_data->_winding, it->_realWinding)) {
			uint32_t vertex = 0;

			it->foreachOnFace([&](HalfEdge &edge) {
				if (vertex < 3) {
					triangle[vertex] = _data->_vertexes[edge.vertex]->_queueIdx + _data->_vertexOffset;
				}
				edge._mark = mark;
				++ vertex;
			});

			if (vertex == 3) {
				res.pushTriangle(res.target, triangle);
			}
		}
	}

	return true;
}

Tesselator::Data::Data(memory::pool_t *p)
: ObjectAllocator(p) { }

void Tesselator::Data::computeInterior() {
	EdgeDict dict(_pool, 8);
	VertexPriorityQueue pq(_pool, _vertexes);

	_edgeDict = &dict;
	_vertexQueue = &pq;

	Vertex *v, *vNext;
	while ((v = pq.extractMin()) != nullptr) {
		for (;;) {
			vNext = pq.getMin();
			if (vNext == NULL || !VertEq(vNext, v, _mathTolerance)) {
				break;
			}

			vNext = pq.extractMin();
			mergeVertexes(v, vNext);
		}

		dict.update(v);

		sweepVertex(pq, dict, v);
	}

	/* Set tess->event for debugging purposes */
	// tess->event = ((ActiveRegion *) dictKey( dictMin( tess->dict )))->eUp->Org;
	// DebugEvent( tess );
	// DoneEdgeDict( tess );
	// DonePriorityQ( tess );

	// tessMeshCheckMesh( tess->mesh );

	_edgeDict = nullptr;
	_vertexQueue = nullptr;
}

void Tesselator::Data::tessellateInterior() {
	auto mark = ++ _markValue;

	for (auto &it : _edgesOfInterests) {
		auto e = it->getEdge();
		if (e->left._mark != mark) {
			uint32_t vertex = 0;

			if (_verbose != VerboseFlag::None) {
				std::cout << "Face: \n";
				e->left.foreachOnFace([&](HalfEdge &edge) {
					std::cout << "\t" << _verbose << vertex ++ << "; " << edge << "\n";
				});
			}

			if (isWindingInside(_winding, e->left._realWinding)) {
				tessellateMonoRegion(&e->left, mark);
			}
		}
		if (e->right._mark != mark) {
			uint32_t vertex = 0;

			if (_verbose != VerboseFlag::None) {
				std::cout << "Face: \n";
				e->right.foreachOnFace([&](HalfEdge &edge) {
					std::cout << "\t" << _verbose << vertex ++ << "; " << edge << "\n";
				});
			}

			if (isWindingInside(_winding, e->right._realWinding)) {
				tessellateMonoRegion(&e->right, mark);
			}
		}
	}
}

bool Tesselator::Data::tessellateMonoRegion(HalfEdge *edge, uint8_t v) {
	edge = removeDegenerateEdges(edge, nullptr);
	if (!edge) {
		return false;
	}

	HalfEdge *up = edge, *lo;

	/* All edges are oriented CCW around the boundary of the region.
	 * First, find the half-edge whose origin vertex is rightmost.
	 * Since the sweep goes from left to right, face->anEdge should
	 * be close to the edge we want.
	 */
	for (; VertLeq(up->getDstVec(), up->getOrgVec()); up = up->getLeftLoopPrev())
		;
	for (; VertLeq(up->getOrgVec(), up->getDstVec()); up = up->getLeftLoopNext())
		;
	lo = up->getLeftLoopPrev();

	//std::cout << "Start: Up: " << *up << "\n";
	//std::cout << "Start: Lo: " << *lo << "\n";

	up->_mark = v;
	lo->_mark = v;

	while (up->getLeftLoopNext() != lo) {
		if (VertLeq(up->getDstVec(), lo->getOrgVec())) {
			//std::cout << "Lo: " << *lo << "\n";
			//std::cout << "Up: " << *up << "\n";

			/* up->Dst is on the left.  It is safe to form triangles from lo->Org.
			 * The EdgeGoesLeft test guarantees progress even when some triangles
			 * are CW, given that the upper and lower chains are truly monotone.
			 */
			while (lo->getLeftLoopNext() != up // invariant is not reached
					&& (lo->getLeftLoopNext()->goesLeft()
							|| Vec2::isCounterClockwise(lo->getOrgVec(), lo->getDstVec(), lo->getLeftLoopNext()->getDstVec()))) {
				auto tempHalfEdge = connectEdges(lo->getLeftLoopNext(), lo);
				if (tempHalfEdge == nullptr) {
					return false;
				}
				_faceEdges.emplace_back(tempHalfEdge);
				lo = tempHalfEdge->sym();
			}
			lo = lo->getLeftLoopPrev();
			lo->_mark = v;
		} else {
			//std::cout << "Up: " << *up << "\n";
			//std::cout << "Lo: " << *lo << "\n";

			/* lo->Org is on the left.  We can make CCW triangles from up->Dst. */
			while (lo->getLeftLoopNext() != up
					&& (up->getLeftLoopPrev()->goesRight()
							|| !Vec2::isCounterClockwise(up->getDstVec(), up->getOrgVec(), up->getLeftLoopPrev()->getOrgVec()))) {
				auto tempHalfEdge = connectEdges(up, up->getLeftLoopPrev());
				if (tempHalfEdge == nullptr) {
					return false;
				}
				_faceEdges.emplace_back(tempHalfEdge);
				up = tempHalfEdge->sym();
			}
			up = up->getLeftLoopNext();
			up->_mark = v;
		}
	}

	/* Now lo->Org == up->Dst == the leftmost vertex.  The remaining region
	 * can be tessellated in a fan from this leftmost vertex.
	 */
	while (lo->getLeftLoopNext()->getLeftLoopNext() != up) {
		auto tempHalfEdge = connectEdges(lo->getLeftLoopNext(), lo);
		if (tempHalfEdge == nullptr) {
			return false;
		}
		_faceEdges.emplace_back(tempHalfEdge);
		lo = tempHalfEdge->sym();
		lo->_mark = v;
	}

	_faceEdges.emplace_back(lo);
	return true;
}

void Tesselator::Data::sweepVertex(VertexPriorityQueue &pq, EdgeDict &dict, Vertex *v) {
	Vec2 tmp;
	IntersectionEvent event;

	auto doConnectEdges = [&] (HalfEdge *source, HalfEdge *target) {
		if (_verbose != VerboseFlag::None) {
			std::cout << "\t\tConnect: \n\t\t\t" << *source << "\n\t\t\t" << *target << "\n";
		}
		auto eNew = connectEdges(source->getLeftLoopPrev(), target);
		_edgesOfInterests.emplace_back(eNew);
	};

	auto onVertex = [&] (VertexType type, Edge *fullEdge, HalfEdge *e, HalfEdge *eNext) {
		auto ePrev = e->getLeftLoopPrev();
		switch (type) {
		case VertexType::Start:
			// 1. Insert e(i) in T and set helper(e, i) to v(i).
			if (!fullEdge->node) {
				fullEdge->node = dict.push(fullEdge, e->_realWinding);
				fullEdge->node->helper = Helper{e, eNext, type };
			}
			fullEdge->node->helper = Helper{e, eNext, type };
			break;
		case VertexType::End:
			// 1. if helper(e, i-1) is a merge vertex
			// 2. 	then Insert the diagonal connecting v(i) to helper(e, i~1) in T.
			// 3. Delete e(i-1) from T.
			if (auto dictNode = ePrev->getEdge()->node) {
				if (dictNode->helper.type == VertexType::Merge) {
					doConnectEdges(e, dictNode->helper.e1);
				}

				// dict.pop(dictNode);
				// ePrev->getEdge()->node = nullptr;
			}
			break;
		case VertexType::Split:
			// 1. Search in T to find the edge e(j) directly left of v(i)
			// 2. Insert the diagonal connecting v(i) to helper(e, j) in D.
			// 3. helper(e, j) <— v(i)
			// 4. Insert e(i) in T and set helper(e, i) to v(i)
			if (auto edgeBelow = dict.getEdgeBelow(e->_originNext->getEdge())) {
				if (edgeBelow->helper.e1) {
					doConnectEdges(e, edgeBelow->helper.e1);
					edgeBelow->helper = Helper{e, eNext, type };
				}
			}
			if (!fullEdge->node) {
				fullEdge->node = dict.push(fullEdge, e->_realWinding);
			}
			fullEdge->node->helper = Helper{e, eNext, type };
			break;
		case VertexType::Merge:
			// 1. if helper(e, i-1) is a merge vertex
			// 2. 	then Insert the diagonal connecting v, to helper(e, i-1) in CD.
			// 3. Delete e(i - 1) from T.
			if (auto dictNode = ePrev->getEdge()->node) {
				if (dictNode->helper.type == VertexType::Merge) {
					doConnectEdges(e, dictNode->helper.e1);
				}

				// dict.pop(dictNode);
				// ePrev->getEdge()->node = nullptr;
			}

			// 4. Search in T to find the edge e(j) directly left of v(i)
			// 5. if helper(e, j) is a merge vertex
			// 6. 	then Insert the diagonal connecting v, to helper(e, j) in D.
			// 7. helper(e, j) <— v(i)
			if (auto edgeBelow = dict.getEdgeBelow(e->_originNext->getEdge())) {
				if (edgeBelow->helper.type == VertexType::Merge) {
					doConnectEdges(e, edgeBelow->helper.e1);
				}
				edgeBelow->helper = Helper{e, eNext, type };
			}
			break;
		case VertexType::RegularBottom: // boundary above vertex
			// 2. if helper(e, i-1) is a merge vertex
			// 3. 	then Insert the diagonal connecting v, to helper(e, i-1) in D
			// 4. Delete e(i-1) from T.
			// 5. Insert e(i) in T and set helper(e, i) to v(i)
			if (auto dictNode = ePrev->getEdge()->node) {
				if (dictNode->helper.type == VertexType::Merge) {
					doConnectEdges(e, dictNode->helper.e1);
				}

				dict.pop(dictNode);
				ePrev->getEdge()->node = nullptr;
			}
			if (!fullEdge->node) {
				fullEdge->node = dict.push(fullEdge, e->_realWinding);
			}
			fullEdge->node->helper = Helper{e, eNext, type };
			break;
		case VertexType::RegularTop: // boundary below vertex
			// 6. Search in T to find the edge e(j) directly left of v(i)
			// 7. if helper(e, j) is a merge vertex
			// 8. 	then Insert the diagonal connecting v(i) to helper(e, j) in D.
			// 9. helper(e, j) <- v(i)
			if (auto edgeBelow = dict.getEdgeBelow(e->_originNext->getEdge())) {
				std::cout << edgeBelow->edge->left << "\n";
				if (edgeBelow->helper.type == VertexType::Merge) {
					doConnectEdges(e, edgeBelow->helper.e1);
				}
				std::cout << "\t\t" << *e << "\n";
				edgeBelow->helper = Helper{e, eNext, type };
			}
			break;
		}
	};

	if (_verbose != VerboseFlag::None) {
		std::cout << "Sweep event: " << v->_origin << "\n";
	}

	VertexType type;
	HalfEdge * e = v->_edge, *eEnd = v->_edge, *eNext = nullptr;
	Edge *fullEdge = nullptr;

	// first - process intersections
	// Intersection can split some edge in dictionary with event vertex,
	// so, event vertex will no longer be valid for iteration
	do {
		e->getEdge()->updateInfo();
		fullEdge = e->getEdge();
		if (e->goesRight()) {
			// push outcoming edge
			if (auto node = dict.checkForIntersects(e, tmp, event, _mathTolerance)) {
				// edges in dictionary should be still valid
				// intersections preserves left subedge, and no
				// intersection points can be at the left of sweep line
				processIntersect(v, node, e, tmp, event);
			}
		}
		e = e->_originNext;
	} while (e != v->_edge);

	// rotate to first left non-convex angle counterclockwise
	// its critical for correct winding calculations
	eEnd = e = getFirstEdge(v);

	do {
		fullEdge = e->getEdge();

		// save original next to prevent new edges processing
		// new edges always added between e and eNext around origin
		eNext = e->_originNext;

		if (e->goesRight()) {
			if (e->_originNext->goesRight()) {
				if (AngleIsConvex(e, e->_originNext)) {
					// winding can be taken from edge below bottom (next) edge
					// or 0 if there is no edges below
					auto edgeBelow = dict.getEdgeBelow(e->_originNext->getEdge());
					if (!edgeBelow) {
						e->_realWinding = e->_originNext->_realWinding = 0;
					} else {
						e->_realWinding = e->_originNext->sym()->_realWinding = edgeBelow->windingAbove;
					}

					std::cout << "\tright-convex: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
						<< " = " << e->_realWinding;

					type = VertexType::Split;
					if (isWindingInside(_winding, e->_realWinding)) {
						std::cout << "; Split\n";
						onVertex(VertexType::Split, fullEdge, e, e->_originNext);
					} else {
						std::cout << "\n";
					}
				} else {
					_edgesOfInterests.emplace_back(e);

					e->_realWinding = e->_originNext->sym()->_realWinding = e->sym()->_realWinding + e->sym()->_winding;

					std::cout << "\tright: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
						<< " = " << e->_realWinding;

					type = VertexType::Start;
					if (isWindingInside(_winding, e->_realWinding)) {
						std::cout << "; Start\n";
						onVertex(VertexType::Start, fullEdge, e, e->_originNext);
					} else {
						std::cout << "\n";
					}
				}
			} else {
				// right-to-left
				e->_realWinding = e->_originNext->sym()->_realWinding;

				std::cout << "\tright-to-left: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
					<< " = " << e->_realWinding;

				type = VertexType::RegularBottom;
				if (isWindingInside(_winding, e->_realWinding)) {
					std::cout << "; RegularBottom\n";
					onVertex(VertexType::RegularBottom, fullEdge, e, e->_originNext);
				} else {
					std::cout << "\n";
				}
			}

			// std::cout << "\t\tpush edge" << fullEdge->getLeftVec() << " - " << fullEdge->getRightVec()
			//		<< " winding: " << e->_realWinding << "\n";

			// push outcoming edge
			if (!fullEdge->node) {
				fullEdge->node = dict.push(fullEdge, e->_realWinding);
				if (isWindingInside(_winding, e->_realWinding)) {
					fullEdge->node->helper = Helper{e, e->_originNext, type };
				}
			}
		} else {
			// remove incoming edge
			if (e->_originNext->goesRight()) {
				// left-to-right
				e->_originNext->sym()->_realWinding = e->_realWinding;

				std::cout << "\tleft-to-right: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
					<< " = " << e->_realWinding;

				type = VertexType::RegularTop;
				if (isWindingInside(_winding, e->_realWinding)) {
					std::cout << "; RegularTop\n";
					onVertex(VertexType::RegularTop, fullEdge, e, e->_originNext);
				} else {
					std::cout << "\n";
				}

			} else {
				if (AngleIsConvex(e, e->_originNext)) {
					std::cout << "\tleft-convex: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
						<< " = " << e->_realWinding;

					type = VertexType::Merge;
					if (isWindingInside(_winding, e->_realWinding)) {
						std::cout << "; Merge\n";
						onVertex(VertexType::Merge, fullEdge, e, e->_originNext);
					} else {
						std::cout << "\n";
					}

				} else {
					std::cout << "\tleft: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
						<< " = " << e->_realWinding;

					type = VertexType::End;
					if (isWindingInside(_winding, e->_realWinding)) {
						std::cout << "; End\n";
						onVertex(VertexType::End, fullEdge, e, e->_originNext);
					} else {
						std::cout << "\n";
					}
				}
			}

			if (fullEdge->node) {
				dict.pop(fullEdge->node);
				fullEdge->node = nullptr;
			}
		}
		e = eNext;
	} while( e != eEnd );

	v->_queueIdx = _exportVertexes.size();
	_exportVertexes.emplace_back(v);
}

HalfEdge *Tesselator::Data::processIntersect(Vertex *v, const EdgeDictNode *edge1, HalfEdge *edge2, Vec2 &intersect, IntersectionEvent ev) {
	std::cout << "Intersect: " << edge1->org << " - " << edge1->dst() << "  X  "
			<< edge2->getOrgVec() << " - " << edge2->getDstVec() << " = " << intersect << "\n";

	auto fixDictEdge = [&] (const EdgeDictNode *e) {
		auto &org = e->edge->getOrgVec();
		auto &dst = e->edge->getDstVec();
		auto tmp = const_cast<EdgeDictNode *>(e);
		if (e->edge->inverted) {
			tmp->norm = org - dst;
			tmp->value.z = org.x;
			tmp->value.w = org.y;
		} else {
			tmp->norm = dst - org;
			tmp->value.z = dst.x;
			tmp->value.w = dst.y;
		}
	};

	auto checkRecursive = [&] (HalfEdge *e) {
		if (auto node = _edgeDict->checkForIntersects(e, intersect, ev, _mathTolerance)) {
			processIntersect(v, node, e, intersect, ev);
		}
	};

	Vertex *vertex = nullptr;

	switch (ev) {
	case IntersectionEvent::Regular:
		// split both edge1 and edge2, recursive check on new edge2 segments
		vertex = splitEdge(edge1->edge->inverted ? &edge1->edge->right : &edge1->edge->left, edge2, intersect);
		fixDictEdge(edge1);
		checkRecursive(edge2);
		_vertexQueue->insert(vertex);
		break;
	case IntersectionEvent::EventIsIntersection:
		// two cases: edges overlap or edge2 starts on edge1
		// in either cases we just split edge1
		// if edges is overlapping - it will be processed when new edge1 segment checked for intersections
		break;
	case IntersectionEvent::EdgeOverlap1:
		// overlap: starts on event, ends on Edge1 (Edge1 smaller then Edge2)
		// split Edge2, merge segment with Edge1 no recursive checks required
		break;
	case IntersectionEvent::EdgeOverlap2:
		// overlap: starts on event, ends on Edge2 (Edge1 larger then Edge2)
		// split Edge1, merge segment with Edge2, no recursive checks required
		break;
	case IntersectionEvent::EdgeConnection1:
		// Edge1 ends somewhere on Edge2
		// split Edge2, perform recursive checks on new segment
		break;
	case IntersectionEvent::EdgeConnection2:
		// Edge2 ends somewhere on Edge1
		// split Edge1, no recursive checks required
		break;
	}

	return edge2;
}

Edge *Tesselator::Data::makeEdgeLoop(const Vec2 &origin) {
	Edge *edge = allocEdge();

	makeVertex(&edge->left)->_origin = origin;
	edge->right.copyOrigin(&edge->left);

	edge->left.origin = edge->right.origin = origin;
	edge->left._leftNext = &edge->left; edge->left._originNext = &edge->right;
	edge->right._leftNext = &edge->right; edge->right._originNext = &edge->left;

	return edge;
}

Vertex *Tesselator::Data::makeVertex(HalfEdge *eOrig) {
	Vertex *vNew = allocVertex();
	vNew->insertBefore(eOrig);
	return vNew;
}

Face *Tesselator::Data::makeFace(HalfEdge *eOrig, Face *fNext) {
	Face *fNew = allocFace();
	fNew->insertBefore(eOrig, fNext);
	return fNew;
}

HalfEdge *Tesselator::Data::pushVertex(HalfEdge *e, const Vec2 &origin, bool clockwise) {
	if (!e) {
		/* Make a self-loop (one vertex, one edge). */
		auto edge = makeEdgeLoop(origin);

		edge->left._winding = (clockwise ? -1 : 1);
		edge->right._winding = (clockwise ? 1 : -1);
		e = &edge->left;
	} else {
		// split primary edge

		Edge *eNew = allocEdge(); // make new edge pair
		Vertex *v = makeVertex(&eNew->left); // make _sym as origin, because _leftNext will be clockwise
		v->_origin = origin;

		HalfEdge::splitEdgeLoops(e, &eNew->left, v);
	}

	if (origin.x < _bmin.x) _bmin.x = origin.x;
	if (origin.x > _bmax.x) _bmax.x = origin.x;
	if (origin.y < _bmin.y) _bmin.y = origin.y;
	if (origin.y > _bmax.y) _bmax.y = origin.y;

	++ _nvertexes;

    return e;
}

HalfEdge *Tesselator::Data::connectEdges(HalfEdge *eOrg, HalfEdge *eDst) {
	// for triangle cut - eDst->lnext = eOrg
	Edge *edge = allocEdge();
	HalfEdge *eNew = &edge->left; // make new edge pair
	HalfEdge *eNewSym = eNew->sym();
	HalfEdge *ePrev = eDst->_originNext->sym();
	HalfEdge *eNext = eOrg->_leftNext;

	eNew->_realWinding = eNewSym->_realWinding = eOrg->_realWinding;

	eNew->copyOrigin(eOrg->sym());
	eNew->sym()->copyOrigin(eDst);

	ePrev->_leftNext = eNewSym; eNewSym->_leftNext = eNext; // external left chain
	eNew->_leftNext = eDst; eOrg->_leftNext = eNew; // internal left chain

	eNew->_originNext = eOrg->sym(); eNext->_originNext = eNew; // org vertex chain
	eNewSym->_originNext = ePrev->sym(); eDst->_originNext = eNewSym; // dst vertex chain

	std::cout << "Connect: " << *eNew << "\n";

	edge->updateInfo();

	return eNew;
}

Vertex *Tesselator::Data::splitEdge(HalfEdge *eOrg1, HalfEdge *eOrg2, const Vec2 &vec2) {
	Vertex *v = nullptr;
	HalfEdge *oPrevOrg = nullptr;
	HalfEdge *oPrevNew = nullptr;

	const Edge *fullEdge1 = eOrg1->getEdge();
	const Edge *fullEdge2 = eOrg2->getEdge();

	// swap edges if eOrg2 will be upper then eOrg1
	if (fullEdge2->direction > fullEdge1->direction) {
		auto tmp = eOrg2;
		eOrg2 = eOrg1;
		eOrg1 = tmp;
	}

	do {
		// split primary edge
		HalfEdge *eNew = &allocEdge()->left; // make new edge pair
		v = makeVertex(eNew); // make _sym as origin, because _leftNext will be clockwise
		v->_origin = vec2;

		auto v2 = _vertexes[eOrg1->sym()->vertex];

		std::cout << *eOrg1 << "\n";
		HalfEdge::splitEdgeLoops(eOrg1, eNew, v);
		std::cout << *eNew << "\n";

		if (v2->_edge == eOrg1->sym()) {
			v2->_edge = eNew->sym();
		}

		oPrevOrg = eNew;
		oPrevNew = eOrg1->sym();

		eNew->getEdge()->updateInfo();
	} while (0);

	do {
		auto v2 = _vertexes[eOrg2->sym()->vertex];

		// split and join secondary edge
		HalfEdge *eNew = &allocEdge()->left; // make new edge pair

		HalfEdge::splitEdgeLoops(eOrg2, eNew, v);
		HalfEdge::joinEdgeLoops(eOrg2, oPrevOrg);
		HalfEdge::joinEdgeLoops(eNew->sym(), oPrevNew);

		if (v2->_edge == eOrg2->sym()) {
			v2->_edge = eNew->sym();
		}

		eNew->getEdge()->updateInfo();
	} while (0);

	return v;
}

// rotate to first left non-convex angle counterclockwise
HalfEdge *Tesselator::Data::getFirstEdge(Vertex *v) const {
	auto e = v->_edge;
	do {
		if (e->goesRight()) {
			if (e->_originNext->goesRight()) {
				if (AngleIsConvex(e, e->_originNext)) {
					// convex right angle is solution
					return e;
				} else {
					// non-convex right angle, skip
				}
			} else {
				// right-to-left angle, next angle is solution
				return e->_originNext;
			}
		} else {
			if (e->_originNext->goesLeft()) {
				if (AngleIsConvex(e, e->_originNext)) {
					// convex left angle, next angle is solution
					return e->_originNext;
				} else {
					// non-convex left angle, skip
				}
			} else {
				// left-to-right angle, skip
			}
		}
		e = e->_originNext;
	} while ( e != v->_edge );
	return e;
}

void Tesselator::Data::mergeVertexes(Vertex *org, Vertex *merge) {
	if (_verbose != VerboseFlag::None) {
		std::cout << _verbose << "Merge:\n\t" << *org << "\n\t" << *merge << "\n";
	}

	auto mergeEdges = [&] (HalfEdge *l, HalfEdge *r) {

	};

	auto insertNext = [&] (HalfEdge *l, HalfEdge *r) {
		auto lNext = l->_originNext;

		// std::cout << "Merge l: " << *l << "\n";
		// std::cout << "Merge r: " << *r << "\n";
		// std::cout << "Merge n: " << *lNext << "\n";

		if (r->_originNext != r) {
			auto rOriginPrev = r->getOriginPrev();
			auto rLeftPrev = r->getLeftLoopPrev();

			rOriginPrev->_originNext = r->_originNext;
			rLeftPrev->_leftNext = r->_leftNext;
		}

		r->_originNext = lNext; r->sym()->_leftNext = l;
		lNext->sym()->_leftNext = r; l->_originNext = r;
	};

	// rotate to first left non-convex angle counterclockwise
	auto eOrg = org->_edge;
	auto eMerge = merge->_edge;
	auto eMergeEnd = eMerge;

	float lA = EdgeAngle(eOrg->getDstVec(), eOrg->getOriginNext()->getDstVec());

	do {
		auto eMergeNext = eMerge->_originNext;

		if (eMerge->sym()->vertex == org->_uniqueIdx) {
			org->_edge = removeEdge(eMerge);
			releaseVertex(merge);
			if (_verbose != VerboseFlag::None) {
				std::cout << _verbose << "Out:\n\t" << *org << "\n";
			}
			return;
		}

		eMerge = eMergeNext;
	} while (eMerge != eMergeEnd);

	do {
		auto eMergeNext = eMerge->_originNext;

		do {
			auto rA = EdgeAngle(eOrg->getDstVec(), eMerge->getDstVec());
			if (EdgeAngleIsBelowTolerance(rA, _mathTolerance)) {
				mergeEdges(eOrg, eMerge);
				break;
			} else if (rA < lA) {
				insertNext(eOrg, eMerge);
				eMerge->origin = eOrg->origin;
				eMerge->vertex = eOrg->vertex;
				lA = rA;
				break;
			} else {
				eOrg = eOrg->_originNext;
				lA = EdgeAngle(eOrg->getDstVec(), eOrg->getOriginNext()->getDstVec());
			}
		} while (true);

		eMerge = eMergeNext;
	} while (eMerge != eMergeEnd);

	releaseVertex(merge);

	/*org->foreach([&] (const HalfEdge &e) {
		std::cout << "Edge:\n";
		e.foreachOnFace([&](const HalfEdge &edge) {
			std::cout << edge << "\n";
		});
	});*/

	if (_verbose != VerboseFlag::None) {
		std::cout << _verbose << "Out:\n\t" << *org << "\n";
	}
}

HalfEdge *Tesselator::Data::removeEdge(HalfEdge *e) {
	auto eSym = e->sym();

	auto eLeftPrev = e->getLeftLoopPrev();
	auto eSymLeftPrev = eSym->getLeftLoopPrev();
	auto eOriginPrev = e->getOriginPrev();
	auto eSymOriginPrev = eSym->getOriginPrev();

	eLeftPrev->_leftNext = e->_leftNext;
	eSymLeftPrev->_leftNext = eSym->_leftNext;

	eOriginPrev->_originNext = eSym->_originNext;
	eSymOriginPrev->_originNext = e->_originNext;

	releaseEdge(e->getEdge());

	return eSymOriginPrev->_originNext;
}

HalfEdge * Tesselator::Data::removeDegenerateEdges(HalfEdge *e, uint32_t *nedges) {
	auto eEnd = e;

	do {
		auto eLnext = e->_leftNext;

		if (VertEq(e->getOrgVec(), e->getDstVec(), _mathTolerance) && e->_leftNext->_leftNext != e) {
			if (eEnd == e) {
				eEnd = eLnext;
			}
			auto vertex = _vertexes[e->sym()->vertex];
			auto merge = _vertexes[e->vertex];
			vertex->_edge = removeEdge(e);
			releaseVertex(merge);

			e = eLnext;
			eLnext = e->_leftNext;

			if (nedges) {
				-- *nedges;
			}
		}
		if (eLnext->_leftNext == e) {
			// Degenerate contour (one or two edges)

			if (eLnext != e) {
				releaseVertex(_vertexes[eLnext->vertex]);
				releaseVertex(_vertexes[eLnext->sym()->vertex]);
				releaseEdge(eLnext->getEdge());
				if (nedges) {
					-- *nedges;
				}
			}
			releaseVertex(_vertexes[e->vertex]);
			releaseVertex(_vertexes[e->sym()->vertex]);
			releaseEdge(e->getEdge());
			if (nedges) {
				-- *nedges;
			}
			return nullptr; // no more face
		}

		e = e->_leftNext;
	} while (e != eEnd);

	return eEnd;
}

}
