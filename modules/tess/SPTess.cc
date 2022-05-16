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
	memory::vector<HalfEdge *> _edgesOfInterests;
	memory::vector<HalfEdge *> _faceEdges;
	Vec2 _bmax, _bmin;

	TessResult *_result = nullptr;
	EdgeDict* _edgeDict = nullptr;
	VertexPriorityQueue *_vertexQueue = nullptr;

	Winding _winding = Winding::NonZero;
	float _antialiasValue = 0.0f;
	uint32_t _nvertexes = 0;
	uint8_t _markValue = 0;

	Data(memory::pool_t *p);

	void removeDegenerateEdges();
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

	void markFaces();
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

void Tesselator::pushVertex(Cursor &cursor, const Vec2 &vertex) {
	if (!cursor.closed) {
		cursor.edge = _data->pushVertex(cursor.edge, vertex, cursor.isClockwise);
	}
}

void Tesselator::closeContour(Cursor &cursor) {
	if (cursor.closed) {
		return;
	}

	cursor.closed = true;

	std::cout << "Contour:\n";
	cursor.edge->foreachOnFace([&] (HalfEdge &e) {
		std::cout << "\t" << e.getOrgVec() << " -> " << e.getDstVec() << "\n";
	});
}

bool Tesselator::prepare(TessResult &res) {
	_data->_result = &res;
	_data->computeInterior();
	_data->tessellateInterior();
	_data->markFaces();
	_data->_result = nullptr;
	return false;
}

bool Tesselator::write(TessResult &res) {
	return false;
}

Tesselator::Data::Data(memory::pool_t *p)
: ObjectAllocator(p) { }

void Tesselator::Data::removeDegenerateEdges() {
	//HalfEdge *eLnext;

	/*for (auto e : _edges) {
		eLnext = e->left._leftNext;

		if (VertEq(e->getOrgVec(), e->getDstVec()) && e->left._leftNext->_leftNext != &e->left) {
			// Zero-length edge, contour has at least 3 edges
			spliceEdges(eLnext, &e->left); // deletes e->Org
			deleteEdge(&e->left); // e is a self-loop
			e = eLnext->getEdge();
			eLnext = e->left._leftNext;
		}
		if (eLnext->_leftNext == &e->left) {
			// Degenerate contour (one or two edges)

			if (eLnext != &e->left) {
				deleteEdge(eLnext);
			}
			deleteEdge(&e->left);
		}
	}*/
}

void Tesselator::Data::computeInterior() {
	removeDegenerateEdges();

	EdgeDict dict(_pool, 8);
	VertexPriorityQueue pq(_pool, _vertexes);

	_edgeDict = &dict;
	_vertexQueue = &pq;

	Vertex *v, *vNext;
	while ((v = pq.extractMin()) != nullptr) {
		for (;;) {
			vNext = pq.getMin();
			if (vNext == NULL || !VertEq(vNext, v)) {
				break;
			}

			/* Merge together all vertices at exactly the same location.
			* This is more efficient than processing them one at a time,
			* simplifies the code (see ConnectLeftDegenerate), and is also
			* important for correct handling of certain degenerate cases.
			* For example, suppose there are two identical edges A and B
			* that belong to different contours (so without this code they would
			* be processed by separate sweep events).  Suppose another edge C
			* crosses A and B from above.  When A is processed, we split it
			* at its intersection point with C.  However this also splits C,
			* so when we insert B we may compute a slightly different
			* intersection point.  This might leave two edges with a small
			* gap between them.  This kind of error is especially obvious
			* when using boundary extraction (TESS_BOUNDARY_ONLY).
			*/
			vNext = pq.extractMin();
			// spliceEdges(v->_edge, vNext->_edge);
		}

		dict.update(v);

		sweepVertex(pq, dict, v);
	}

	/* Set tess->event for debugging purposes */
	// tess->event = ((ActiveRegion *) dictKey( dictMin( tess->dict )))->eUp->Org;
	// DebugEvent( tess );
	// DoneEdgeDict( tess );
	// DonePriorityQ( tess );

	removeDegenerateEdges();
	// tessMeshCheckMesh( tess->mesh );

	_edgeDict = nullptr;
	_vertexQueue = nullptr;
}

void Tesselator::Data::tessellateInterior() {
	auto mark = ++ _markValue;

	for (auto &it : _edgesOfInterests) {
		auto e = it->getEdge();
		if (e->left._mark != mark) {
			if (isWindingInside(_winding, e->left._realWinding)) {
				tessellateMonoRegion(&e->left, mark);
			} else {
				/*uint32_t vertex = 0;
				e->left.foreachOnFace([&](HalfEdge &edge) {
					edge._mark = mark;
					std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
				});*/
			}
		}
		if (e->right._mark != mark) {
			if (isWindingInside(_winding, e->right._realWinding)) {
				tessellateMonoRegion(&e->right, mark);
			} else {
				/*uint32_t vertex = 0;
				e->right.foreachOnFace([&](HalfEdge &edge) {
					edge._mark = mark;
					std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
				});*/
			}
		}
	}
}

bool Tesselator::Data::tessellateMonoRegion(HalfEdge *edge, uint8_t v) {
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

	up->_mark = v;
	lo->_mark = v;

	while (up->getLeftLoopNext() != lo) {
		if (VertLeq(up->getDstVec(), lo->getOrgVec())) {
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
	}

	_faceEdges.emplace_back(lo);
	return true;
}

void Tesselator::Data::sweepVertex(VertexPriorityQueue &pq, EdgeDict &dict, Vertex *v) {
	Vec2 tmp;
	IntersectionEvent event;

	auto isConvex = [&] (HalfEdge *a, HalfEdge *b) {
		return a->getEdge()->direction > b->getEdge()->direction;
	};

	std::cout << "Sweep event: " << v->_origin << "\n";
	HalfEdge * e = v->_edge;
	HalfEdge *eEnd = e;
	Edge *fullEdge = nullptr;

	// first - process intersections
	// Intersection can split some edge in dictionary with event vertex,
	// so, event vertex will no longer be valid for iteration
	do {
		e->getEdge()->updateInfo();
		fullEdge = e->getEdge();
		if (e->goesRight()) {
			// push outcoming edge
			if (auto node = dict.checkForIntersects(e, tmp, event)) {
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

	// pre-initialize
	e = v->_edge;

	do {
		fullEdge = e->getEdge();
		// std::cout << "\tEdge: " << fullEdge->getLeftVec() << " - " << fullEdge->getRightVec()
		//		<< " " << fullEdge->inverted << " " << fullEdge->direction << "\n";
		if (e->goesRight()) {
			if (e->_originNext->goesRight()) {
				if (isConvex(e, e->_originNext)) {
					// convex right angle is solution
					eEnd = e;
					break;
				} else {
					// non-convex right angle, skip
				}
			} else {
				// right-to-left angle, next angle is solution
				e = eEnd = e->_originNext;
				break;
			}
		} else {
			if (e->_originNext->goesLeft()) {
				if (isConvex(e, e->_originNext)) {
					// convex left angle, next angle is solution
					e = eEnd = e->_originNext;
					break;
				} else {
					// non-convex left angle, skip
				}
			} else {
				// left-to-right angle, skip
			}
		}
		e = e->_originNext;
	} while ( e != v->_edge );

	do {
		fullEdge = e->getEdge();

		if (e->goesRight()) {
			if (e->_originNext->goesRight()) {
				if (isConvex(e, e->_originNext)) {
					// winding can be taken from edge below bottom (next) edge
					// or 0 if there is no edges below
					auto edgeBelow = dict.getEdgeBelow(e->_originNext->getEdge());
					if (!edgeBelow) {
						e->_realWinding = e->_originNext->_realWinding = 0;
					} else {
						e->_realWinding = e->_originNext->sym()->_realWinding = edgeBelow->windingAbove;
					}

					// std::cout << "\tright-convex: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
					//		<< " = " << e->_realWinding << "\n";
				} else {
					_edgesOfInterests.emplace_back(e);

					e->_realWinding = e->_originNext->sym()->_realWinding = e->sym()->_realWinding + e->sym()->_winding;

					// std::cout << "\tright: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
					//		<< " = " << e->_realWinding << "\n";
				}
			} else {
				// right-to-left
				e->_realWinding = e->_originNext->sym()->_realWinding;

				// std::cout << "\tright-to-left: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
				//		<< " = " << e->_realWinding << "\n";
			}

			// std::cout << "\t\tpush edge" << fullEdge->getLeftVec() << " - " << fullEdge->getRightVec()
			//		<< " winding: " << e->_realWinding << "\n";

			// push outcoming edge
			fullEdge->node = dict.push(fullEdge, e->_realWinding);

		} else {
			if (e->_originNext->goesRight()) {
				// left-to-right
				e->_originNext->sym()->_realWinding = e->_realWinding;

				// std::cout << "\tleft-to-right: " << e->getDstVec() << " - " << e->getOrgVec() << " - " << e->_originNext->getDstVec()
				//			<< " = " << e->_realWinding << "\n";
			}

			// remove incoming edge
			if (fullEdge->node) {
				dict.pop(fullEdge->node);
				fullEdge->node = nullptr;
			}
		}
		e = e->_originNext;
	} while( e != eEnd );
}

HalfEdge *Tesselator::Data::processIntersect(Vertex *v, const EdgeDictNode *edge1, HalfEdge *edge2, Vec2 &intersect, IntersectionEvent ev) {
	// std::cout << "Intersect: " << edge1->org << " - " << edge1->dst() << " X "
	//		<< edge2->getOrgVec() << " - " << edge2->getDstVec() << " = " << intersect << "\n";

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
		// std::cout << *e << "\n";
		if (auto node = _edgeDict->checkForIntersects(e, intersect, ev)) {
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
	HalfEdge *eNew = &allocEdge()->left; // make new edge pair
	HalfEdge *eNewSym = eNew->sym();
	HalfEdge *ePrev = eDst->_originNext->sym();
	HalfEdge *eNext = eOrg->_leftNext;

	eNew->copyOrigin(eOrg->sym());
	eNew->sym()->copyOrigin(eDst);

	ePrev->_leftNext = eNewSym; eNewSym->_leftNext = eNext; // external left chain
	eNew->_leftNext = eDst; eOrg->_leftNext = eNew; // internal left chain

	eNew->_originNext = eOrg->sym(); eNext->_originNext = eNew; // org vertex chain
	eNewSym->_originNext = ePrev->sym(); eDst->_originNext = eNewSym; // dst vertex chain

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

		HalfEdge::splitEdgeLoops(eOrg1, eNew, v);

		oPrevOrg = eNew;
		oPrevNew = eOrg1->sym();
	} while (0);

	do {
		// split and join secondary edge
		HalfEdge *eNew = &allocEdge()->left; // make new edge pair

		HalfEdge::splitEdgeLoops(eOrg2, eNew, v);
		HalfEdge::joinEdgeLoops(eOrg2, oPrevOrg);
		HalfEdge::joinEdgeLoops(eNew->sym(), oPrevNew);
	} while (0);

	return v;
}

void Tesselator::Data::markFaces() {
	uint32_t face = 0;
	auto mark = ++ _markValue;

	for (auto &it : _faceEdges) {
		auto e = it->getEdge();
		if (e->left._mark != mark) {
			uint32_t vertex = 0;
			std::cout << "Face: " << face ++ << "\n";
			e->left.foreachOnFace([&](HalfEdge &edge) {
				edge._mark = mark;
				std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
			});
		}
		if (e->right._mark != mark) {
			uint32_t vertex = 0;
			std::cout << "Face: " << face ++ << " (sym)\n";
			e->right.foreachOnFace([&](HalfEdge &edge) {
				edge._mark = mark;
				std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
			});
		}
	}

	/*for (auto e : _edges) {
		if (e->left._mark != mark) {
			uint32_t vertex = 0;
			std::cout << "Face: " << face ++ << "\n";
			e->left.foreachOnFace([&](HalfEdge &edge) {
				edge._mark = mark;
				std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
			});
		}
		if (e->right._mark != mark) {
			uint32_t vertex = 0;
			std::cout << "Face: " << face ++ << " (sym)\n";
			e->right.foreachOnFace([&](HalfEdge &edge) {
				edge._mark = mark;
				std::cout << "\t" << vertex ++ << " " << edge.origin << " " << edge._realWinding << "\n";
			});
		}
	}*/
}

}
