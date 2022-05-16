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

#ifndef MODULES_THREADS_SPEVENTTASKQUEUE_H_
#define MODULES_THREADS_SPEVENTTASKQUEUE_H_

#include "SPThreadTask.h"

namespace stappler::thread {

class EventTaskQueue : public RefBase<memory::StandartInterface> {
public:
	struct Data;

	using Ref = RefBase<memory::StandartInterface>;

	EventTaskQueue(StringView name = StringView());
	~EventTaskQueue();

	void perform(Rc<Task> &&task, bool first = false);
	void perform(std::function<void()> &&, Ref * = nullptr, bool first = false);

	void update(uint32_t *count);

	void onMainThread(Rc<Task> &&task);
	void onMainThread(std::function<void()> &&func, Ref *target);

	// maxOf<uint32_t> - set id to next available
	bool spawnWorkers(uint32_t threadId, uint16_t threadCount);
	bool cancelWorkers();

	void performAll();
	bool waitForAll(TimeInterval = TimeInterval::seconds(1));

	bool wait(uint32_t *count = nullptr);
	bool wait(TimeInterval, uint32_t *count = nullptr);

	StringView getName() const;
	size_t getOutputCounter() const;

	void lock();
	void unlock();

protected:
	Data *_data = nullptr;
};

}

#endif /* MODULES_THREADS_SPEVENTTASKQUEUE_H_ */
