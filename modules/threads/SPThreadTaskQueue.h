/**
Copyright (c) 2019 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef MODULES_THREADS_SPTHREADTASKQUEUE_H_
#define MODULES_THREADS_SPTHREADTASKQUEUE_H_

#include "SPThreadTask.h"

namespace stappler::thread {

class Worker;

class TaskQueue : public RefBase<memory::StandartInterface> {
public:
	using Ref = RefBase<memory::StandartInterface>;

	enum class Flags {
		None = 0,

		// allow to submit task for specific thread via `perform(Map<uint32_t, Vector<Rc<Task>>> &&)`
		// it requires additional space for internal local queues and less optimal thread scheduling
		// with `condition_variable_any` instead of `condition_variable`
		LocalQueue = 1,

		// allow queue to be externally cancelled with `performAll` and `waitForAll`
		// it requires extra condition to track count of tasks executing
		Cancelable = 2,

		// allow to wait for event on queue's main thread via 'wait'
		Waitable = 4,
	};

	struct WorkerContext;

	static const TaskQueue *getOwner();

	TaskQueue(StringView name = StringView(), std::function<void()> &&wakeup = std::function<void()>());
	~TaskQueue();

	void finalize();

	void performAsync(Rc<Task> &&task);

	void perform(Rc<Task> &&task, bool first = false);
	void perform(std::function<void()> &&, Ref * = nullptr, bool first = false);

	bool perform(std::map<uint32_t, std::vector<Rc<Task>>> &&tasks);

	void update(uint32_t *count = nullptr);

	void onMainThread(Rc<Task> &&task);
	void onMainThread(std::function<void()> &&func, Ref *target);

	bool spawnWorkers(Flags);

	// maxOf<uint32_t> - set id to next available
	bool spawnWorkers(Flags, uint32_t threadId, uint16_t threadCount, StringView name = StringView());
	bool cancelWorkers();

	void performAll(Flags flags);
	bool waitForAll(TimeInterval = TimeInterval::seconds(1));

	bool wait(uint32_t *count = nullptr);
	bool wait(TimeInterval, uint32_t *count = nullptr);

	void lock();
	void unlock();

	StringView getName() const { return _name; }

	std::vector<std::thread::id> getThreadIds() const;

	size_t getOutputCounter() const { return _outputCounter.load(); }

protected:
	friend class Worker;

	Rc<Task> popTask(uint32_t idx);
	void onMainThreadWorker(Rc<Task> &&task);

	WorkerContext *_context = nullptr;

	std::mutex _inputMutexQueue;
	std::mutex _inputMutexFree;
	memory::PriorityQueue<Rc<Task>> _inputQueue;

	std::mutex _outputMutex;
	std::vector<Rc<Task>> _outputQueue;
	std::vector<Pair<std::function<void()>, Rc<Ref>>> _outputCallbacks;

	std::atomic<size_t> _outputCounter = 0;
	std::atomic<size_t> _tasksCounter = 0;

	StringView _name = StringView("TaskQueue");

	std::function<void()> _wakeup;
};

SP_DEFINE_ENUM_AS_MASK(TaskQueue::Flags)

}

#endif /* MODULES_THREADS_SPTHREADTASKQUEUE_H_ */
