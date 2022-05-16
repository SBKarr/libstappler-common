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

#include "SPEventTaskQueue.h"
#include "SPThread.h"
#include "SPTime.h"
#include "SPLog.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <fcntl.h>
#include <unistd.h>

namespace stappler::thread {

struct EventTaskWorker;

struct ThreadInfoData {
	StringView name;
	uint32_t managerId = 0;
	uint32_t workerId = 0;
};

struct EventTaskQueue::Data : memory::AllocPool {
	struct Client {
		int fd = -1;
	    struct epoll_event event;
	};

	static void destroy(Data *);

	Data(memory::pool_t *p, StringView);
	~Data();

	bool run(uint32_t threadId, uint32_t nWorkers = std::thread::hardware_concurrency());

	void retain();
	void release();
	bool finalize();

	void pushTask(Rc<Task> &&, bool first);
	Rc<Task> popTask();

	void onMainThread(Rc<Task> &&task);
	void onMainThread(std::function<void()> &&func, Ref *target);
	void onMainThreadWorker(Rc<Task> &&task);

	void wakeup(bool noMoreTasksFlag);

	bool update(uint32_t *count);
	bool wait(TimeInterval);

	size_t getWorkersCount() const { return _workers.size(); }

	uint32_t _nWorkers = std::thread::hardware_concurrency();
	std::atomic<bool> _finalized;
	std::atomic<int32_t> _refCount;

	memory::vector<EventTaskWorker *> _workers;
	memory::pool_t *_pool = nullptr;

	Client _eventEvent;
	std::atomic<size_t> _taskCounter;
	std::mutex _inputMutexQueue;
	std::mutex _inputMutexFree;
	memory::PriorityQueue<Rc<Task>> _inputQueue;

	int _pipe[2] = { -1, -1 };
	int _eventFdWorkers = -1;
	int _eventFdMain = -1;
	int _epollFd = -1;
	ThreadInfoData _info;

	std::mutex _outputMutex;
	std::vector<Rc<Task>> _outputQueue;
	std::vector<Pair<std::function<void()>, Rc<Ref>>> _outputCallbacks;
	std::atomic<size_t> _outputCounter = 0;
};

struct EventTaskWorker : public ThreadInterface<memory::PoolInterface> {
	struct Buffer;
	struct Client;
	struct Generation;

	struct Client {
		int fd = -1;
	    struct epoll_event event;
	};

	static constexpr size_t MaxEvents = 8;

	EventTaskWorker(ThreadInfoData, EventTaskQueue::Data *queue, int pipe, int event);
	~EventTaskWorker();

	virtual void threadInit() override;
	virtual void threadDispose() override;
	virtual bool worker() override;

	bool poll(int);

	std::thread &thread() { return _thread; }

	void runTask(Rc<Task> &&);

	memory::pool_t *_pool = nullptr;
	EventTaskQueue::Data *_queue = nullptr;
	std::thread::id _threadId;

	int _cancelFd = -1;
	int _eventFd = -1;
	ThreadInfoData _info;

	std::thread _thread;
};

static StringView s_getSignalName(int sig) {
	switch (sig) {
	case SIGINT: return "SIGINT";
	case SIGILL: return "SIGILL";
	case SIGABRT: return "SIGABRT";
	case SIGFPE: return "SIGFPE";
	case SIGSEGV: return "SIGSEGV";
	case SIGTERM: return "SIGTERM";
	case SIGHUP: return "SIGHUP";
	case SIGQUIT: return "SIGQUIT";
	case SIGTRAP: return "SIGTRAP";
	case SIGKILL: return "SIGKILL";
	case SIGBUS: return "SIGBUS";
	case SIGSYS: return "SIGSYS";
	case SIGPIPE: return "SIGPIPE";
	case SIGALRM: return "SIGALRM";
	case SIGURG: return "SIGURG";
	case SIGSTOP: return "SIGSTOP";
	case SIGTSTP: return "SIGTSTP";
	case SIGCONT: return "SIGCONT";
	case SIGCHLD: return "SIGCHLD";
	case SIGTTIN: return "SIGTTIN";
	case SIGTTOU: return "SIGTTOU";
	case SIGPOLL: return "SIGPOLL";
	case SIGXCPU: return "SIGXCPU";
	case SIGXFSZ: return "SIGXFSZ";
	case SIGVTALRM: return "SIGVTALRM";
	case SIGPROF: return "SIGPROF";
	case SIGUSR1: return "SIGUSR1";
	case SIGUSR2: return "SIGUSR2";
	default: return "(unknown)";
	}
	return StringView();
}

static bool EventTaskQueue_setNonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		stappler::log::vtext("EventTaskQueue", "fcntl() fail to get flags for ", fd);
		return false;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		stappler::log::vtext("EventTaskQueue", "fcntl() setNonblocking failed for ", fd);
		return false;
	}
	return true;
}

EventTaskQueue::EventTaskQueue(StringView name)  {
	auto pool = memory::pool::create(memory::PoolFlags::None);
	memory::pool::push(pool);
	_data = new (pool) Data(pool, name);
	memory::pool::pop();
}

EventTaskQueue::~EventTaskQueue() {
	_data->release();
	_data = nullptr;
}

void EventTaskQueue::perform(Rc<Task> &&task, bool first) {
	_data->pushTask(move(task), first);
}

void EventTaskQueue::perform(std::function<void()> &&cb, Ref *ref, bool first) {
	perform(Rc<Task>::create([fn = move(cb)] (const Task &) -> bool {
		fn();
		return true;
	}, nullptr, ref), first);
}

void EventTaskQueue::update(uint32_t *count = nullptr) {
	_data->update(count);
}

void EventTaskQueue::onMainThread(Rc<Task> &&task) {
	_data->onMainThread(move(task));
}

void EventTaskQueue::onMainThread(std::function<void()> &&func, Ref *target) {
	_data->onMainThread(move(func), target);
}

// maxOf<uint32_t> - set id to next available
bool EventTaskQueue::spawnWorkers(uint32_t threadId, uint16_t threadCount) {
	if (threadId == maxOf<uint32_t>()) {
		threadId = getNextThreadId();
	}

	return _data->run(threadId, threadCount);
}

bool EventTaskQueue::cancelWorkers() {
	return _data->finalize();
}

void EventTaskQueue::performAll() {
	spawnWorkers(maxOf<uint32_t>(), std::thread::hardware_concurrency());
	waitForAll();
	cancelWorkers();
}

bool EventTaskQueue::waitForAll(TimeInterval iv) {
	return false; // todo
}

bool EventTaskQueue::wait(uint32_t *count) {
	if (_data->wait(TimeInterval())) {
		return _data->update(count);
	}
	return false;
}

bool EventTaskQueue::wait(TimeInterval iv, uint32_t *count) {
	if (_data->wait(iv)) {
		return _data->update(count);
	}
	return false;
}

StringView EventTaskQueue::getName() const {
	return _data->_info.name;
}

size_t EventTaskQueue::getOutputCounter() const {
	return _data->_outputCounter.load();
}

void EventTaskQueue::lock() {
	_data->_outputMutex.lock();
}

void EventTaskQueue::unlock() {
	_data->_outputMutex.unlock();
}

void EventTaskQueue::Data::destroy(Data *data) {
	auto pool = data->_pool;
	delete data;
	memory::pool::destroy(pool);
}

EventTaskQueue::Data::Data(memory::pool_t *p, StringView name)
: _finalized(false), _refCount(1), _pool(p) {
	_inputQueue.setQueueLocking(_inputMutexQueue);
	_inputQueue.setFreeLocking(_inputMutexFree);

	_info.name = name.pdup(_pool);
	_eventFdWorkers = ::eventfd(0, EFD_NONBLOCK);
	_eventFdMain = ::eventfd(0, EFD_NONBLOCK);
	_taskCounter.store(0);

	_eventEvent.fd = _eventFdMain;
	_eventEvent.event.data.ptr = &_eventEvent;
	_eventEvent.event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;
	_epollFd = epoll_create1(0);
	if (_epollFd >= 0) {
		int err = 0;

		err = epoll_ctl(_epollFd, EPOLL_CTL_ADD, _eventFdMain, &_eventEvent.event);
		if (err == -1) {
			char buf[256] = { 0 };
			std::cout << "Failed to add eventfd epoll_ctl("
					<< _eventFdMain << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
		}
	}
}

EventTaskQueue::Data::~Data() {
	if (_pipe[0] > -1) { ::close(_pipe[0]); }
	if (_pipe[1] > -1) { ::close(_pipe[1]); }
	if (_eventFdWorkers > -1) { ::close(_eventFdWorkers); }
	if (_eventFdMain > -1) { ::close(_eventFdMain); }
	if (_epollFd > -1) { ::close(_epollFd); }

	_workers.clear();
}

bool EventTaskQueue::Data::run(uint32_t threadId, uint32_t nWorkers) {
	if (!_workers.empty() || _finalized) {
		return false;
	}

	memory::pool::push(_pool);
	_info.managerId = threadId;
	_nWorkers = nWorkers;
	_workers.reserve(_nWorkers);
	if (pipe2(_pipe, O_NONBLOCK) == 0) {
		EventTaskQueue_setNonblocking(_pipe[0]);
		EventTaskQueue_setNonblocking(_pipe[1]);

		for (uint32_t i = 0; i < _nWorkers; i++) {
			ThreadInfoData info(_info);
			info.workerId = i;
			EventTaskWorker *worker = new (_pool) EventTaskWorker(info, this, _pipe[0], _eventFdWorkers);
			_workers.push_back(worker);
		}
	}

	memory::pool::pop();
	return true;
}

void EventTaskQueue::Data::retain() {
	_refCount ++;
}

void EventTaskQueue::Data::release() {
	if (--_refCount <= 0) {
		destroy(this);
	}
}

bool EventTaskQueue::Data::finalize() {
	if (_workers.empty() || _finalized) {
		return false;
	}

	_finalized = true;
	if (write(_pipe[1], "END!", 4) > 0) {
		for (auto &it : _workers) {
			if (it->thread().joinable()) {
				it->thread().join();
				delete it;
			}
		}
	}
	return true;
}

void EventTaskQueue::Data::pushTask(Rc<Task> &&task, bool first) {
	if (!task->prepare()) {
		task->setSuccessful(false);
		onMainThread(std::move(task));
		return;
	}

	++ _taskCounter;
	_inputQueue.push(task->getPriority().get(), first, std::move(task));

	uint64_t value = 1;
	if (::write(_eventFdWorkers, &value, sizeof(uint64_t)) != 8) {
		return;
	}
}

Rc<Task> EventTaskQueue::Data::popTask() {
	Rc<Task> ret;
	_inputQueue.pop_direct([&] (memory::PriorityQueue<Rc<Task>>::PriorityType, Rc<Task> &&task) {
		ret = move(task);
	});
	return ret;
}

void EventTaskQueue::Data::onMainThread(Rc<Task> &&task) {
    if (!task) {
        return;
    }

    _outputMutex.lock();
    _outputQueue.push_back(std::move(task));
    ++ _outputCounter;
	_outputMutex.unlock();
	wakeup(_taskCounter.load() == 0);
}

void EventTaskQueue::Data::onMainThread(std::function<void()> &&func, Ref *target) {
    _outputMutex.lock();
    _outputCallbacks.emplace_back(std::move(func), target);
    ++ _outputCounter;
	_outputMutex.unlock();
	wakeup(_taskCounter.load() == 0);
}

void EventTaskQueue::Data::onMainThreadWorker(Rc<Task> &&task) {
    if (!task) {
        return;
    }

	if (!task->getCompleteTasks().empty()) {
		_outputMutex.lock();
		_outputQueue.push_back(std::move(task));
	    ++ _outputCounter;
		_outputMutex.unlock();
		wakeup(_taskCounter.fetch_sub(1) == 1);
	} else {
		if (_taskCounter.fetch_sub(1) == 1) {
			wakeup(false);
		}
	}
}

void EventTaskQueue::Data::wakeup(bool noMoreTasksFlag) {
	uint64_t value = 1;
	if (::write(_eventFdMain, &value, sizeof(uint64_t)) != 8) {
		return;
	}
}

bool EventTaskQueue::Data::update(uint32_t *count) {
    _outputMutex.lock();

    std::vector<Rc<Task>> stack = std::move(_outputQueue);
	std::vector<Pair<std::function<void()>, Rc<Ref>>> callbacks = std::move(_outputCallbacks);

	_outputQueue.clear();
	_outputCallbacks.clear();
	_outputCounter.store(0);
	_outputMutex.unlock();

	for (auto &task : stack) {
		task->onComplete();
	}

	for (auto &task : callbacks) {
		task.first();
	}

    if (count) {
    	*count += stack.size() + callbacks.size();
    }
    return true;
}

bool EventTaskQueue::Data::wait(TimeInterval iv) {
	uint64_t value = 0;
	auto sz = ::read(_eventFdMain, &value, sizeof(uint64_t));

	if (sz == 8) {
		return true;
	}

	std::array<struct epoll_event, EventTaskWorker::MaxEvents> _events;

	while (true) {
		int nevents = ::epoll_wait(_epollFd, _events.data(), EventTaskWorker::MaxEvents, iv ? iv.toMillis() : -1);
		if (nevents == -1 && errno != EINTR) {
			char buf[256] = { 0 };
			stappler::log::vtext("EventTaskQueue", "epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")");
			return false;
		} else {
			return false; // timeout
		}

		/// process high-priority events
		for (int i = 0; i < nevents; i++) {
			Client *client = (Client *)_events[i].data.ptr;

			if ((_events[i].events & EPOLLIN)) {
				if (client->fd == _eventFdMain) {
					uint64_t value = 0;
					auto sz = ::read(_eventFdMain, &value, sizeof(uint64_t));
					if (sz == 8 && value) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

EventTaskWorker::EventTaskWorker(ThreadInfoData info, EventTaskQueue::Data *queue, int pipe, int event)
: _queue(queue), _cancelFd(pipe), _eventFd(event), _info(info), _thread(EventTaskWorker::workerThread, this, queue) {
	_queue->retain();
}

EventTaskWorker::~EventTaskWorker() {
	if (_queue) {
		_queue->release();
	}
}

void EventTaskWorker::threadInit() {
	_threadId = std::this_thread::get_id();
	memory::pool::initialize();
	_pool = memory::pool::createTagged(_info.name.data(), memory::PoolFlags::None);

	ThreadInfo::setThreadInfo(_info.managerId, _info.workerId, _info.name, true);
}

void EventTaskWorker::threadDispose() {
	memory::pool::destroy(_pool);
	memory::pool::terminate();

	if (_queue) {
		auto tmp = _queue;
		_queue = nullptr;
		tmp->release();
	}
}

bool EventTaskWorker::worker() {
	Client pipeEvent;
	pipeEvent.fd = _cancelFd;
	pipeEvent.event.data.ptr = &pipeEvent;
	pipeEvent.event.events = EPOLLIN | EPOLLET;

	Client eventEvent;
	eventEvent.fd = _eventFd;
	eventEvent.event.data.ptr = &eventEvent;
	eventEvent.event.events = EPOLLIN | EPOLLET | EPOLLEXCLUSIVE;

	sigset_t sigset;
	sigfillset(&sigset);

	int signalFd = ::signalfd(-1, &sigset, 0);
	EventTaskQueue_setNonblocking(signalFd);

	int epollFd = epoll_create1(0);

	int err = 0;

	err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _cancelFd, &pipeEvent.event);
	if (err == -1) {
		char buf[256] = { 0 };
		std::cout << "Failed to start thread worker with pipe epoll_ctl("
				<< _cancelFd << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
	}

	err = epoll_ctl(epollFd, EPOLL_CTL_ADD, _eventFd, &eventEvent.event);
	if (err == -1) {
		char buf[256] = { 0 };
		std::cout << "Failed to start thread worker with eventfd epoll_ctl("
				<< _eventFd << ", EPOLL_CTL_ADD): " << strerror_r(errno, buf, 255) << "\n";
	}

	while (poll(epollFd)) {
		struct signalfd_siginfo si;
		int nr = ::read(signalFd, &si, sizeof si);
		while (nr == sizeof si) {
			if (si.ssi_signo != SIGINT) {
				stappler::log::vtext("EventTaskQueue",
						"epoll_wait() exit with signal: ", si.ssi_signo, " ", s_getSignalName(si.ssi_signo));
			}
			nr = ::read(signalFd, &si, sizeof si);
		}
	}

	close(signalFd);
	close(epollFd);

	return false;
}

bool EventTaskWorker::poll(int epollFd) {
	bool _shouldClose = false;
	std::array<struct epoll_event, EventTaskWorker::MaxEvents> _events;

	while (!_shouldClose) {
		int nevents = epoll_wait(epollFd, _events.data(), EventTaskWorker::MaxEvents, -1);
		if (nevents == -1 && errno != EINTR) {
			char buf[256] = { 0 };
			stappler::log::vtext("EventTaskQueue", "epoll_wait() failed with errno ", errno, " (", strerror_r(errno, buf, 255), ")");
			return false;
		} else if (errno == EINTR) {
			return true;
		}

		/// process high-priority events
		for (int i = 0; i < nevents; i++) {
			Client *client = (Client *)_events[i].data.ptr;

			if ((_events[i].events & EPOLLIN)) {
				if (client->fd == _eventFd) {
					uint64_t value = 0;
					auto sz = ::read(_eventFd, &value, sizeof(uint64_t));
					if (sz == 8 && value) {
						auto ev = _queue->popTask();
						if (value > 1) {
							value -= 1;
							// forward event to another worker
							if (::write(_eventFd, &value, sizeof(uint64_t)) != 8) {
								stappler::log::vtext("EventTaskQueue", "fail to forward event");
							}
						}
						if (ev) {
							runTask(move(ev));
						}
					}
				}
			}
		}

		for (int i = 0; i < nevents; i++) {
			Client *client = (Client *)_events[i].data.ptr;
			if ((_events[i].events & EPOLLIN)) {
				if (client->fd == _cancelFd) {
					_shouldClose = true;
				} else if (client->fd == _eventFd) {
					// do nothing
				}
			}
		}
	}

	return !_shouldClose;
}

void EventTaskWorker::runTask(Rc<Task> &&task) {
	bool success = false;
	do {
		memory::pool::push(_pool);
		success = task->execute();
		memory::pool::pop();
		memory::pool::clear(_pool);
	} while (0);

	task->setSuccessful(success);
	_queue->onMainThreadWorker(std::move(task));
}

}
