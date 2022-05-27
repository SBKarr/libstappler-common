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

#include "SPCommon.h"
#include "SPTessLine.h"

namespace stappler::app {

SP_EXTERN_C int _spMain(argc, argv) {

	auto mempool = memory::pool::create();

	/*do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;
		result.pushVertex = [] (void *, uint32_t idx, const geom::Vec2 &pt, float vertexValue) {
			std::cout << "Vertex: " << idx << ": " << pt << "\n";
		};
		result.pushTriangle = [] (void *, uint32_t points[3]) {
			std::cout << "Face: " << points[0] << " " << points[1] << " " << points[2] << "\n";
		};

		auto fillTess = Rc<geom::Tesselator>::create(mempool);
		auto strokeTess = nullptr;

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(0, 0);
		line.drawLine(50, 50);
		line.drawLine(50, 50);
		line.drawLine(0, 0);
		line.drawClose(true);

		fillTess->prepare(result);
		fillTess->write(result);
	} while (0);

	do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;
		result.pushVertex = [] (void *, uint32_t idx, const geom::Vec2 &pt, float vertexValue) {
			std::cout << "Vertex: " << idx << ": " << pt << "\n";
		};
		result.pushTriangle = [] (void *, uint32_t points[3]) {
			std::cout << "Face: " << points[0] << " " << points[1] << " " << points[2] << "\n";
		};

		auto fillTess = Rc<geom::Tesselator>::create(mempool);
		auto strokeTess = nullptr;

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(0, 0);
		line.drawLine(50, 50);
		line.drawLine(-50, 50);
		line.drawClose(true);
		line.drawBegin(0, 0);
		line.drawLine(-50, -50);
		line.drawLine(50, -50);
		line.drawClose(true);

		fillTess->prepare(result);
		fillTess->write(result);
	} while (0);

	do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;
		result.pushVertex = [] (void *, uint32_t idx, const geom::Vec2 &pt, float vertexValue) {
			std::cout << "Vertex: " << idx << ": " << pt << "\n";
		};
		result.pushTriangle = [] (void *, uint32_t points[3]) {
			std::cout << "Face: " << points[0] << " " << points[1] << " " << points[2] << "\n";
		};

		auto fillTess = Rc<geom::Tesselator>::create(mempool);
		auto strokeTess = nullptr;

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(100, 150);
		line.drawLine(100, 0);
		line.drawLine(0, 0);
		line.drawLine(0, 150);
		line.drawClose(true);

		fillTess->prepare(result);
		fillTess->write(result);
	} while (0);*/


	do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;
		result.pushVertex = [] (void *, uint32_t idx, const geom::Vec2 &pt, float vertexValue) {
			std::cout << "Vertex: " << idx << ": " << pt << "\n";
		};
		result.pushTriangle = [] (void *, uint32_t points[3]) {
			std::cout << "Face: " << points[0] << " " << points[1] << " " << points[2] << "\n";
		};

		auto fillTess = Rc<geom::Tesselator>::create(mempool);
		auto strokeTess = nullptr;

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		float x = 0.0f; float y = 0.0f;
		float rx = 100.0f; float ry = 100.0f;

		line.drawBegin(x + rx, y);
		line.drawArc(rx, ry, 0, false, false, x, y - ry);
		line.drawArc(rx, ry, 0, false, false, x - rx, y);
		line.drawArc(rx, ry, 0, false, false, x, y + ry);
		line.drawArc(rx, ry, 0, false, false, x + rx, y);
		line.drawClose(true);

		fillTess->prepare(result);
		fillTess->write(result);
	} while (0);


	/*do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;

		auto fillTess = Rc<geom::Tesselator>::create(mempool);
		auto strokeTess = nullptr;

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(100, 150);
		line.drawLine(0, 0);
		line.drawLine(100, 0);
		line.drawLine(0, 150);
		line.drawClose(true);

		fillTess->prepare(result);
	} while (0);*/

	/*do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;

		auto fillTess = nullptr;
		auto strokeTess = Rc<geom::Tesselator>::create(mempool);

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(100, 150);
		line.drawLine(0, 0);
		line.drawLine(100, 0);
		line.drawLine(0, 150);
		line.drawClose(true);

		strokeTess->prepare(result);
		strokeTess->write(result);
	} while (0);

	memory::pool::clear(mempool);

	do {
		memory::pool::context ctx(mempool);

		geom::TessResult result;

		auto fillTess = nullptr;
		auto strokeTess = Rc<geom::Tesselator>::create(mempool);

		geom::LineDrawer line(1.0f, Rc<geom::Tesselator>(fillTess), Rc<geom::Tesselator>(strokeTess), 10.0f);

		line.drawBegin(100, 150);
		line.drawLine(0, 0);
		line.drawLine(100, 0);
		line.drawLine(0, 150);
		line.drawClose(true);

		strokeTess->prepare(result);
		strokeTess->write(result);
	} while (0);*/

	return 0;
}

}
