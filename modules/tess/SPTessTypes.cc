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

#include "SPTessTypes.h"
#include "SPLog.h"

namespace stappler::geom {

int TessVerboseInfo = std::ios_base::xalloc();

bool EdgeDictNode::operator < (const EdgeDictNode &other) const {
	if (value.y == other.value.y) {
		return edge->direction < other.edge->direction;
	} else {
		return value.y < other.value.y;
	}
}

bool EdgeDictNode::operator < (const Edge &other) const {
	auto &left = other.getLeftVec();
	if (value.y == left.y) {
		return edge->direction < other.direction; // dst.y
	} else {
		return value.y < left.y;
	}
}

bool EdgeDictNode::operator <= (const EdgeDictNode &other) const {
	if (value.y == other.value.y) {
		return value.w == other.value.w || edge->direction < other.edge->direction; // dst.y
	} else {
		return value.y < other.value.y;
	}
}

bool EdgeDictNode::operator== (const EdgeDictNode &other) const {
	return value.y == other.value.y && value.w == other.value.w;
}

void Vertex::insertBefore(HalfEdge *eOrig) {
	_edge = eOrig;

	/* fix other edges on this vertex loop */
	HalfEdge *e = eOrig;
	do {
		e->setOrigin(this);
		e = e->_originNext;
	} while( e != eOrig );
}

void Vertex::removeFromList(Vertex *newOrg) {
	HalfEdge *e, *eStart = _edge;
	e = eStart;
	do {
		e->setOrigin(newOrg);
		e = e->_originNext;
	} while( e != eStart );
}

void Vertex::foreach(const Callback<void(const HalfEdge &)> &cb) const {
	auto e = _edge;
	do {
		cb(*e);
		e = e->_originNext;
	} while( e != _edge );
}

/* MakeFace( newFace, eOrig, fNext ) attaches a new face and makes it the left
* face of all edges in the face loop to which eOrig belongs.  "fNext" gives
* a place to insert the new face in the global face list.  We insert
* the new face *before* fNext so that algorithms which walk the face
* list will not see the newly created faces. */
void Face::insertBefore(HalfEdge *eOrig, Face *fNext) {
	Face *fPrev = fNext->_prev;
	this->_prev = fPrev;
	this->_next = fNext;
	fPrev->_next = this;
	fNext->_prev = this;

	_edge = eOrig;
	_inside = fNext->_inside;

	/* fix other edges on this face loop */
	HalfEdge *e = eOrig;
	do {
		//e->_leftFace = this;
		e = e->_leftNext;
	} while( e != eOrig );
}

/* KillFace( fDel ) destroys a face and removes it from the global face
* list.  It updates the face loop to point to a given new face. */
void Face::removeFromList(Face *newLFace) {
	HalfEdge *e, *eStart = _edge;

	/* change the left face of all affected edges */
	e = eStart;
	do {
		//e->_leftFace = newLFace;
		e = e->_leftNext;
	} while( e != eStart );

	Face *fPrev = this->_prev;
	Face *fNext = this->_next;
	fNext->_prev = fPrev;
	fPrev->_next = fNext;
}

void HalfEdge::splitEdgeLoops(HalfEdge *eOrg, HalfEdge *eNew, Vertex *v) {
	eNew->sym()->copyOrigin(eOrg->sym());
	eOrg->sym()->setOrigin(v);
	eNew->setOrigin(v);

	HalfEdge *a = eOrg, *b = eOrg->sym(), // original edge
			*c = eNew, *d = eNew->sym(), // new edge
			*e = eOrg->_leftNext, // next edge in left loop
			*g = b->_originNext, *h = g->sym(); // prev edge in right loop

	e->_originNext = d; d->_originNext = g; // vertex cycle around dest vertex;
	c->_originNext = b; b->_originNext = c; // cycle around new vertex;
	a->_leftNext = c; c->_leftNext = e; // left face loop
	h->_leftNext = d; d->_leftNext = b; // right face loop
	c->_winding = a->_winding; d->_winding = b->_winding;
}

void HalfEdge::joinEdgeLoops(HalfEdge *eOrg, HalfEdge *oPrev) {
	// connect eOrg into vertex
	HalfEdge *a = eOrg, *b = eOrg->sym(), // original edge
				*e = oPrev, // next edge in left loop
				*g = oPrev->_originNext, *h = g->sym(); // prev edge in right loop

	e->_originNext = b; b->_originNext = g; // cycle around new vertex;
	a->_leftNext = e; h->_leftNext = b; // left and right loops
}

HalfEdge *HalfEdge::sym() const {
	return (HalfEdge *)((char *)this - sizeof(HalfEdge) * isRight);
}

uint32_t HalfEdge::getIndex() const {
	return ((uintptr_t)this >> 5) % 1024;
}

void HalfEdge::setOrigin(const Vertex *v) {
	origin = v->_origin;
	vertex = v->_uniqueIdx;
}

void HalfEdge::copyOrigin(const HalfEdge *e) {
	origin = e->origin;
	vertex = e->vertex;
}

HalfEdge *HalfEdge::getOriginNext() const {
	return _originNext;
}

HalfEdge *HalfEdge::getOriginPrev() const {
	return sym()->_leftNext;
}

HalfEdge *HalfEdge::getDestinationNext() const {
	return sym()->_originNext->sym();
}

HalfEdge *HalfEdge::getDestinationPrev() const {
	return _leftNext->sym();
}

HalfEdge *HalfEdge::getLeftLoopNext() const {
	return _leftNext;
}

HalfEdge *HalfEdge::getLeftLoopPrev() const {
	return _originNext->sym();
}

HalfEdge *HalfEdge::getRightLoopNext() const {
	return sym()->_leftNext->sym();
}

HalfEdge *HalfEdge::getRightLoopPrev() const {
	return sym()->_originNext;
}

const Vec2 &HalfEdge::getOrgVec() const {
	return origin;
}

const Vec2 &HalfEdge::getDstVec() const {
	return sym()->origin;
}

Edge *HalfEdge::getEdge() const {
	return (Edge *)((char *)this - sizeof(HalfEdge) * edgeOffset);
}

bool HalfEdge::goesLeft() const {
	return ((Edge *)((char *)this - sizeof(HalfEdge) * edgeOffset))->inverted != static_cast<bool>(edgeOffset);
}

bool HalfEdge::goesRight() const {
	return ((Edge *)((char *)this - sizeof(HalfEdge) * edgeOffset))->inverted == static_cast<bool>(edgeOffset);
}

void HalfEdge::foreachOnFace(const Callback<void(HalfEdge &)> &cb) {
	auto e = this;
	do {
		cb(*e);
		e = e->_leftNext;
	} while( e != this );
}

void HalfEdge::foreachOnVertex(const Callback<void(HalfEdge &)> &cb) {
	auto e = this;
	do {
		cb(*e);
		e = e->_originNext;
	} while( e != this );
}

void HalfEdge::foreachOnFace(const Callback<void(const HalfEdge &)> &cb) const {
	auto e = this;
	do {
		cb(*e);
		e = e->_leftNext;
	} while( e != this );
}

void HalfEdge::foreachOnVertex(const Callback<void(const HalfEdge &)> &cb) const {
	auto e = this;
	do {
		cb(*e);
		e = e->_originNext;
	} while( e != this );
}

float HalfEdge::getDirection() const {
	return getEdge()->direction;
}

Edge::Edge() {
	left.isRight = -1;
	left.edgeOffset = 0;
	left._originNext = &left;
	left._leftNext = &right;
	right.isRight = 1;
	right.edgeOffset = 1;
	right._originNext = &right;
	right._leftNext = &left;
}

const Vec2 &Edge::getLeftVec() const {
	return inverted ? right.getOrgVec() : left.getOrgVec();
}

const Vec2 &Edge::getRightVec() const {
	return inverted ? left.getOrgVec() : right.getOrgVec();
}

const Vec2 &Edge::getOrgVec() const {
	return left.origin;
}

const Vec2 &Edge::getDstVec() const {
	return right.origin;
}

void Edge::updateInfo() {
	inverted = !EdgeGoesRight(&left);
	direction = EdgeDirection(getRightVec() - getLeftVec());
};

int16_t Edge::getLeftWinding() const {
	return inverted ? right._realWinding : left._realWinding;
}

int16_t Edge::getRightWinding() const {
	return inverted ? left._realWinding : right._realWinding;
}

ObjectAllocator::ObjectAllocator(memory::pool_t *pool) : _pool(pool), _vertexes(pool) {
	_vertexes.reserve(VertexSetPrealloc);
}

Edge *ObjectAllocator::allocEdge() {
	Edge *edge = nullptr;
	if (!_freeEdges) {
		preallocateEdges(EdgeAllocBatch);
	}

	auto node = _freeEdges;
	_freeEdges = (Edge *)node->node;
	edge = new (node) Edge();

	return edge;
}

Vertex *ObjectAllocator::allocVertex() {
	Vertex *vertex = nullptr;
	if (!_freeVertexes) {
		preallocateVertexes(VertexAllocBatch);
	}

	auto node = _freeVertexes;
	_freeVertexes = (Vertex *)node->_edge;
	vertex = new (node) Vertex();
	vertex->_uniqueIdx = _vertexes.size();

	_vertexes.emplace_back(vertex);

	return vertex;
}

Face *ObjectAllocator::allocFace() {
	Face *face = nullptr;
	if (_freeFaces) {
		auto node = _freeFaces;
		_freeFaces = node->_next;
		face = new (node) Face();
	} else {
		face = new (_pool) Face();
	}
	return face;
}

void ObjectAllocator::releaseEdge(Edge *eDel) {
	removeEdgeFromVec(_edgesOfInterests, &eDel->left);
	removeEdgeFromVec(_edgesOfInterests, &eDel->right);
	removeEdgeFromVec(_faceEdges, &eDel->left);
	removeEdgeFromVec(_faceEdges, &eDel->right);

	eDel->~Edge();

	eDel->node = (EdgeDictNode *)_freeEdges;
	_freeEdges = eDel;
}

void ObjectAllocator::releaseVertex(uint32_t vDelId, uint32_t vNewId) {
	auto it1 = _vertexes[vDelId];
	auto it2 = _vertexes[vNewId];

	if (it1 && it2) {
		auto vDel = it1;

		vDel->removeFromList(it2);
		vDel->~Vertex();
		_vertexes[vDelId] = nullptr;

		vDel->_edge = (HalfEdge *)_freeVertexes;
		_freeVertexes = vDel;
	}
}

void ObjectAllocator::releaseVertex(Vertex *vDel) {
	if (vDel) {
		_vertexes[vDel->_uniqueIdx] = nullptr;
		vDel->~Vertex();

		vDel->_edge = (HalfEdge *)_freeVertexes;
		_freeVertexes = vDel;
	}
}

void ObjectAllocator::trimVertexes() {
	size_t offset = 0;
	for (auto it = _vertexes.rbegin(); it != _vertexes.rend(); ++ it) {
		if (*it == nullptr) {
			++ offset;
		} else {
			break;
		}
	}

	if (offset > 0) {
		_vertexes.resize(_vertexes.size() - offset);
	}
}

void ObjectAllocator::preallocateVertexes(uint32_t n) {
	if (auto vertsMem = (Vertex *)memory::pool::palloc(_pool, sizeof(Vertex) * n)) {
		for (uint32_t i = 0; i < n; ++ i) {
			auto mem = vertsMem + i;
			mem->_edge = (HalfEdge *)(mem + 1);
		}

		Vertex *vtmp = _freeVertexes;
		_freeVertexes = vertsMem;
		(vertsMem + n - 1)->_edge = (HalfEdge *)vtmp;
	}

	_vertexes.reserve(n);
	_exportVertexes.reserve(n);
}

void ObjectAllocator::preallocateEdges(uint32_t n) {
	if (auto edgesMem = (Edge *)memory::pool::palloc(_pool, sizeof(Edge) * n)) {
		for (uint32_t i = 0; i < n; ++ i) {
			auto mem = edgesMem + i;
			mem->node = (EdgeDictNode *)(mem + 1);
		}

		Edge *etmp = _freeEdges;
		_freeEdges = edgesMem;
		(edgesMem + n - 1)-> node = (EdgeDictNode *)(etmp);
	}
}

void ObjectAllocator::removeEdgeFromVec(memory::vector<HalfEdge *> &vec, HalfEdge *e) {
	auto eOIt = std::find(vec.begin(), vec.end(), e);
	if (eOIt != vec.end()) {
		if (e->_leftNext != e) {
			*eOIt = e->_leftNext;
		} else {
			*eOIt = nullptr;
		}
	}
}

VertexPriorityQueue::Heap::Heap(memory::pool_t *p, uint32_t s) : max(s), pool(p) {
	nodes = (Node *)memory::pool::palloc(pool, (max + 1) * sizeof(Node));
	handles = (Elem *)memory::pool::palloc(pool, (max + 1) * sizeof(Elem));

	nodes[1].handle = 1; /* so that Minimum() returns NULL */
	handles[1].key = nullptr;
}

VertexPriorityQueue::Heap::~Heap() {
	memory::pool::free(pool, nodes, (max + 1) * sizeof(Node));
	memory::pool::free(pool, handles, (max + 1) * sizeof(Elem));
}

void VertexPriorityQueue::Heap::init() {
	/* This method of building a heap is O(n), rather than O(n lg n). */

	for (uint32_t i = size; i >= 1; --i) {
		floatDown( i );
	}

	initialized = true;
}

/* returns INV_HANDLE iff out of memory */
VertexPriorityQueue::Handle VertexPriorityQueue::Heap::insert(Key keyNew) {
	uint32_t curr;
	Handle free;

	curr = ++size;
	if ((curr * 2) > max) {
		Node *saveNodes = nodes;
		Elem *saveHandles = handles;

		// If the heap overflows, double its size.
		auto tmpSize = max;

		max <<= 1;

		nodes = (Node*) memory::pool::palloc(pool, ((max + 1) * sizeof(Node)));
		if (nodes != NULL) {
			memcpy(nodes, saveNodes, ((tmpSize + 1) * sizeof(Node)));
		}

		handles = (Elem*) memory::pool::palloc(pool, ((max + 1) * sizeof(Elem)));
		if (handles != NULL) {
			memcpy(handles, saveHandles, (size_t) ((tmpSize + 1) * sizeof(Elem)));
		}

		memory::pool::free(pool, saveNodes, (tmpSize + 1) * sizeof(Node));
		memory::pool::free(pool, saveHandles, (tmpSize + 1) * sizeof(Elem));
	}

	if (freeList == 0) {
		free = curr;
	} else {
		free = freeList;
		freeList = handles[free].node;
	}

	nodes[curr].handle = free;
	handles[free].node = curr;
	handles[free].key = keyNew;

	if (initialized) {
		floatUp(curr);
	}
	SPASSERT(free != InvalidHandle, "pqHeapInsert");
	return free;
}

/* really pqHeapExtractMin */
VertexPriorityQueue::Key VertexPriorityQueue::Heap::extractMin() {
	Node *n = nodes;
	Elem *h = handles;
	Handle hMin = n[1].handle;
	Key min = h[hMin].key;

	if (size > 0) {
		n[1].handle = n[size].handle;
		h[n[1].handle].node = 1;

		h[hMin].key = NULL;
		h[hMin].node = freeList;
		freeList = hMin;

		if (--size > 0) {
			floatDown(1);
		}
	}
	return min;
}

/* really pqHeapDelete */
void VertexPriorityQueue::Heap::remove(Handle hCurr) {
	Node *n = nodes;
	Elem *h = handles;
	uint32_t curr;

	SPASSERT( hCurr >= 1 && hCurr <= Handle(max) && h[hCurr].key != nullptr, "pqHeapDelete");

	curr = h[hCurr].node;
	n[curr].handle = n[size].handle;
	h[n[curr].handle].node = curr;

	if (curr <= --size) {
		if (curr <= 1 || VertLeq(h[n[curr >> 1].handle].key, h[n[curr].handle].key)) {
			floatDown(curr);
		} else {
			floatUp(curr);
		}
	}
	h[hCurr].key = NULL;
	h[hCurr].node = freeList;
	freeList = hCurr;
}


void VertexPriorityQueue::Heap::floatDown(int curr) {
	Node *n = nodes;
	Elem *h = handles;
	Handle hCurr, hChild;
	uint32_t child;

	hCurr = n[curr].handle;
	for (;;) {
		child = curr << 1;
		if (child < size && VertLeq(h[n[child + 1].handle].key, h[n[child].handle].key)) {
			++child;
		}

		SPASSERT(child <= this->max, "FloatDown");

		hChild = n[child].handle;
		if (child > size || VertLeq(h[hCurr].key, h[hChild].key)) {
			n[curr].handle = hCurr;
			h[hCurr].node = curr;
			break;
		}
		n[curr].handle = hChild;
		h[hChild].node = curr;
		curr = child;
	}
}

void VertexPriorityQueue::Heap::floatUp(int curr) {
	Node *n = nodes;
	Elem *h = handles;
	Handle hCurr, hParent;
	uint32_t parent;

	hCurr = n[curr].handle;
	for (;;) {
		parent = curr >> 1;
		hParent = n[parent].handle;
		if (parent == 0 || VertLeq(h[hParent].key, h[hCurr].key)) {
			n[curr].handle = hCurr;
			h[hCurr].node = curr;
			break;
		}
		n[curr].handle = hParent;
		h[hParent].node = curr;
		curr = parent;
	}
}

VertexPriorityQueue::VertexPriorityQueue(memory::pool_t *p, const memory::vector<Vertex *> &vec)
: heap(p, vec.size()), max(vec.size()), pool(p) {
	keys = (Key*) memory::pool::palloc(p, max * sizeof(Key));

	for (auto &v : vec) {
		if (v) {
			v->_queueIdx = insert(v);
			if (v->_queueIdx == InvalidHandle) {
				return;
			}
		}
	}

	if (init()) {
		initialized = true;
	}
}

VertexPriorityQueue::~VertexPriorityQueue() {
	memory::pool::free(pool, keys, max * sizeof(Key));
}


#define LT(x,y)     (! VertLeq(y,x))
#define GT(x,y)     (! VertLeq(x,y))
#define KeySwap(a,b)   if(1){Key *tmp = *a; *a = *b; *b = tmp;}else

bool VertexPriorityQueue::init() {
	Key **p, **r, **i, **j, *piv;
	struct {
		Key **p, **r;
	} Stack[50], *top = Stack;
	unsigned int seed = 2016473283;

	/* Create an array of indirect pointers to the keys, so that we
	 * the handles we have returned are still valid. */
	/*
	 pq->order = (PQkey **)memAlloc( (size_t)
	 (pq->size * sizeof(pq->order[0])) );
	 */
	order = (Key**) memory::pool::palloc(pool, size_t((size + 1) * sizeof(Key*)));

	p = order;
	r = p + size - 1;
	for (piv = keys, i = p; i <= r; ++piv, ++i) {
		*i = piv;
	}

	/* Sort the indirect pointers in descending order,
	 * using randomized Quicksort */
	top->p = p;
	top->r = r;
	++top;
	while (--top >= Stack) {
		p = top->p;
		r = top->r;
		while (r > p + 10) {
			seed = seed * 1539415821 + 1;
			i = p + seed % (r - p + 1);
			piv = *i;
			*i = *p;
			*p = piv;
			i = p - 1;
			j = r + 1;
			do {
				do {
					++i;
				} while (GT(**i, *piv));
				do {
					--j;
				} while (LT(**j, *piv));
				KeySwap(i, j);
			} while (i < j);
			KeySwap(i, j); /* Undo last swap */
			if (i - p < r - j) {
				top->p = j + 1;
				top->r = r;
				++top;
				r = i - 1;
			} else {
				top->p = p;
				top->r = i - 1;
				++top;
				p = j + 1;
			}
		}
		/* Insertion sort small lists */
		for (i = p + 1; i <= r; ++i) {
			piv = *i;
			for (j = i; j > p && LT(**(j - 1), *piv); --j) {
				*j = *(j - 1);
			}
			*j = piv;
		}
	}
	max = size;
	initialized = true;

	heap.init();

#ifndef NDEBUG
	p = order;
	r = p + size - 1;
	for (i = p; i < r; ++i) {
		SPASSERT( VertLeq( **(i+1), **i ), "pqInit");
	}
#endif

	return 1;
}

#undef LT
#undef GT
#undef KeySwap

bool VertexPriorityQueue::empty() const {
	return size == 0 && heap.empty();
}

VertexPriorityQueue::Handle VertexPriorityQueue::insert(Key keyNew) {
	int curr;

	if (initialized) {
		return heap.insert(keyNew);
	}
	curr = size;
	if( ++ size >= max ) {
		Key *saveKey = keys;
		// If the heap overflows, double its size.
		auto tmpSize = max;
		max <<= 1;
		keys = (Key*) memory::pool::palloc(pool, max * sizeof(Key));
		if (keys) {
			memcpy(keys, saveKey, (size_t) (tmpSize * sizeof(Key)));
		}

		memory::pool::free(pool, saveKey, tmpSize * sizeof(Key));
	}
	SPASSERT(curr != InvalidHandle, "pqInsert");
	keys[curr] = keyNew;

	/* Negative handles index the sorted array. */
	return -(curr+1);
}

void VertexPriorityQueue::remove(Handle curr) {
	if (curr >= 0) {
		heap.remove(curr);
		return;
	}
	curr = -(curr + 1);
	SPASSERT(curr < Handle(max) && keys[curr] != nullptr, "pqDelete");

	keys[curr] = nullptr;
	while (size > 0 && *(order[size - 1]) == nullptr) {
		--size;
	}
}

VertexPriorityQueue::Key VertexPriorityQueue::extractMin() {
	Key sortMin, heapMin;

	if (size == 0) {
		return heap.extractMin();
	}
	sortMin = *(order[size - 1]);
	if (!heap.empty()) {
		heapMin = heap.getMin();
		if (VertLeq(heapMin, sortMin)) {
			return heap.extractMin();
		}
	}
	do {
		--size;
	} while (size > 0 && *(order[size - 1]) == NULL);
	return sortMin;
}

VertexPriorityQueue::Key VertexPriorityQueue::getMin() const {
	Key sortMin, heapMin;

	if (size == 0) {
		return heap.getMin();
	}
	sortMin = *(order[size - 1]);
	if (!heap.empty()) {
		heapMin = heap.getMin();
		if (VertLeq(heapMin, sortMin)) {
			return heapMin;
		}
	}
	return sortMin;
}

EdgeDict::EdgeDict(memory::pool_t *p, uint32_t size) : nodes(p), pool(p) {
	nodes.reserve(size);
	nodes.set_memory_persistent(true);
}

const EdgeDictNode * EdgeDict::push(Edge *edge, int16_t windingAbove) {
	const EdgeDictNode *ret;
	auto &dst = edge->getDstVec();
	auto &org = edge->getOrgVec();
	if (org == event) {
		auto norm = dst - event;
		ret = & (*nodes.emplace(EdgeDictNode{
			event, norm,
			Vec4(event.x, event.y, dst.x, dst.y),
			edge, windingAbove,
			std::abs(norm.x) > std::numeric_limits<float>::epsilon()
		}).first);
	} else if (dst == event) {
		auto norm = org - event;
		ret = & (*nodes.emplace(EdgeDictNode{
			event, norm,
			Vec4(event.x, event.y, org.x, org.y),
			edge, windingAbove,
			std::abs(norm.x) > std::numeric_limits<float>::epsilon()
		}).first);
	}

	/*for (auto &it : nodes) {
		std::cout << "Edge: " << it.org << " - " << Vec2(it.value.z, it.value.w)
				<< " - " << Vec2(it.value.x, it.value.y) << " - " << it.windingAbove << "\n";
	}*/

	return ret;
}

void EdgeDict::pop(const EdgeDictNode *node) {
	// std::cout << node->edge->left;
	auto it = nodes.lower_bound(*node);
	auto end = nodes.end();
	while (it != end && *it <= *node && &(*it) != node) {
		// std::cout << it->edge->left;
		++ it;
	}
	if (it != end) {
		nodes.erase(it);
	}
}

void EdgeDict::update(Vertex *v) {
	event = v->_origin;
	for (auto &n : nodes) {
		if (n.horizontal) {
			const float tValue = (event.x - n.org.x) / (n.norm.x);
			n.value.x = n.org.x + n.norm.x * tValue;
			n.value.y = n.org.y + n.norm.y * tValue;
		} else {
			const float sValue = (event.y - n.org.y) / (n.norm.y);
			n.value.x = n.org.x + n.norm.x * sValue;
			n.value.y = n.org.y + n.norm.y * sValue;
		}
	}
}

const EdgeDictNode * EdgeDict::checkForIntersects(HalfEdge *edge, Vec2 &intersectPoint, IntersectionEvent &ev, float tolerance) const {
	if (nodes.empty()) {
		return nullptr;
	}

	auto &org = edge->getOrgVec(); // == event
	auto &dst = edge->getDstVec();

	const Vec2 norm = dst - org;
	const float dir = EdgeDirection(norm);

	auto simdVec1 = simd::load(org.x, org.y, dst.x, dst.y);

	for (auto &n : nodes) {

		// std::cout << "checkForIntersects:\n\t" << *edge << "\n\t" << n.org << " - " << n.value << "\n";

		// overlap check should be made in mergeVertexes
		// so, should never happen
		if (VertEq(n.org, org, tolerance)) {
			if (norm.y == 0.0f && n.norm.y == 0.0f) {
				// overlap, horizontal
				// note, sweepline logic means same direction on X
				if (dst.x < n.dstX()) {
					intersectPoint = dst;
					ev = IntersectionEvent::EdgeOverlap2;
				} else {
					intersectPoint = n.dst();
					ev = IntersectionEvent::EdgeOverlap1;
				}
			} else if (norm.x == 0.0f && n.norm.x == 0.0f) {
				// overlap, vertical
				// note, sweepline logic means same direction on Y for vertical lines
				if (dst.y < n.dstY()) {
					intersectPoint = dst;
					ev = IntersectionEvent::EdgeOverlap2;
				} else {
					intersectPoint = n.dst();
					ev = IntersectionEvent::EdgeOverlap1;
				}
			} else {
				if (std::abs(dir - n.edge->direction) < tolerance) {
					if (dst.x < n.value.z) {
						intersectPoint = dst;
						ev = IntersectionEvent::EdgeOverlap2;
					} else {
						intersectPoint = n.dst();
						ev = IntersectionEvent::EdgeOverlap1;
					}
				}
			}
			continue; // common org, not interested
		} else if (VertEq(n.current(), org, tolerance)) {
			if (VertEq(n.current(), n.dst(), tolerance)) {
				continue; // no intersection, just line end
			}
			intersectPoint = event;
			ev = IntersectionEvent::EventIsIntersection;
			return &n;
		}

		if (VertEq(dst, n.dst(), tolerance)) {
			continue; // common dst
		}

		simd::f32x4 intersect;
		if (simd::isVec2BboxIntersects(simdVec1, simd::load(&n.value.x), intersect)) {
			Vec4 isect; simd::store(&isect.x, intersect);
			const float denom = isect.w * isect.x - isect.z * isect.y; // crossProduct2Vector(A, B, C, D);
			if (denom != 0.0f) {
				const auto CAx = org.x - n.value.x;
				const auto CAy = org.y - n.value.y;

				auto S = (CAy * isect.z - CAx * isect.w) / denom;
				auto T = (CAy * isect.x - CAx * isect.y) / denom;

				//if (S > 0.5f) { S = 1.0f - S; }
				//if (T > 0.5f) { T = 1.0f - T; }

				if (S >= 0.0f && S <= 1.0f && T >= 0.0f && T <= 1.0f) {
					intersectPoint = Vec2(org.x + S * isect.x, org.y + S * isect.y);
					if (VertEq(intersectPoint, dst, tolerance)) {
						ev = IntersectionEvent::EdgeConnection2; // edge ends on n;
					} else if (VertEq(intersectPoint, n.dst(), tolerance)) {
						ev = IntersectionEvent::EdgeConnection1; // n ends on edge;
					} else {
						ev = IntersectionEvent::Regular;
					}
					return &n;
				}
			}
		}
	}

	return nullptr;
}

const EdgeDictNode * EdgeDict::getEdgeBelow(Edge *e) const {
	if (nodes.empty()) {
		return nullptr;
	}

	auto it = nodes.lower_bound(*e); // first not less then e
	if (it == nodes.begin()) {
		// first edge in dict greater or equal then e, no edges below
		return nullptr;
	} else {
		-- it;
		while (it->current() == event) {
			-- it;
		}
		// edge before it is less then e
		return &(*it);
	}
}

std::ostream &operator<<(std::ostream &out, const Vertex &v) {

	switch (VerboseFlag(out.iword( TessVerboseInfo ))) {
	case VerboseFlag::None:
		out << "Vertex (" << v._uniqueIdx << ") : " << v._origin;
		break;
	case VerboseFlag::General:
		out << "Vertex (" << v._uniqueIdx << ") : " << v._origin;
		break;
	case VerboseFlag::Full:
		out << "Vertex (" << v._uniqueIdx << ") : " << v._origin << "\n";
		v.foreach([&] (const HalfEdge &e) {
			Vec2 orgVec = e.origin;
			Vec2 dstVec = e.sym()->origin;
			uint32_t orgIdx = e.vertex;
			uint32_t dstIdx = e.sym()->vertex;

			out << "\tEdge (" << e.getIndex() << ":" << e.sym()->getIndex() << ") : " << orgVec << " - " << dstVec << "\n";
			out << "\t\tDir: (" << e.getIndex() << "; org: " << orgIdx << "; left: " << e._leftNext->getIndex()
					<< "; ccw: " << e._originNext->getIndex() << ")\n";
			out << "\t\tSym: (" << e.sym()->getIndex() << "; org: " << dstIdx << "; left: " << e.sym()->_leftNext->getIndex()
					<< "; ccw: " << e.sym()->_originNext->getIndex() << ")\n";
		});
		break;
	}

	return out;
}

std::ostream &operator<<(std::ostream &out, const HalfEdge &e) {
	Vec2 orgVec = e.origin;
	Vec2 dstVec = e.sym()->origin;
	uint32_t orgIdx = e.vertex;
	uint32_t dstIdx = e.sym()->vertex;

	switch (VerboseFlag(out.iword( TessVerboseInfo ))) {
	case VerboseFlag::None:
		out << "Edge (" << e.getIndex() << ":" << e.sym()->getIndex() << ") : " << orgVec << " - " << dstVec;
		break;
	case VerboseFlag::General:
		out << "Edge (" << e.getIndex() << ":" << e.sym()->getIndex() << ") : " << orgVec << " - " << dstVec
		<< " winding: " << e._realWinding << ":" << e._winding << ";";
		if (e.goesLeft()) {
			out << " goes left;";
		} else if (e.goesRight()) {
			out << " goes right;";
		} else {
			out << " unknown direction;";
		}
		break;
	case VerboseFlag::Full:
		out << "Edge (" << e.getIndex() << ":" << e.sym()->getIndex() << ") : " << orgVec << " - " << dstVec
			<< " winding: " << e._realWinding << ":" << e._winding << ";\n";
		out << "\tDir: (" << e.getIndex() << "; org: " << orgIdx << "; left: " << e._leftNext->getIndex()
				<< "; ccw: " << e._originNext->getIndex() << ")";
		if (e.goesLeft()) {
			out << " goes left;";
		} else if (e.goesRight()) {
			out << " goes right;";
		} else {
			out << " unknown direction;";
		}
		out << "\n";
		out << "\tSym: (" << e.sym()->getIndex() << "; org: " << dstIdx << "; left: " << e.sym()->_leftNext->getIndex()
				<< "; ccw: " << e.sym()->_originNext->getIndex() << ")";
		if (e.sym()->goesLeft()) {
			out << " goes left;";
		} else if (e.sym()->goesRight()) {
			out << " goes right;";
		} else {
			out << " unknown direction;";
		}
		out << "\n";
		break;
	}
	return out;
}

std::ostream &operator<<(std::ostream &stream, VerboseFlag e) {
	stream.iword( TessVerboseInfo ) = toInt(e);
	return stream;
}

}
