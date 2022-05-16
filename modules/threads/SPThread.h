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

#ifndef MODULES_THREADS_SPTHREAD_H_
#define MODULES_THREADS_SPTHREAD_H_

#include "SPCommon.h"
#include "SPRef.h"

namespace stappler::thread {

struct ThreadInfo {
	static constexpr uint32_t mainThreadId = maxOf<uint32_t>() - 1;

	static ThreadInfo *getThreadLocal();
	static void setMainThread();
	static void setThreadInfo(uint32_t, uint32_t, StringView, bool);
	static void setThreadInfo(StringView);

	uint32_t threadId = 0;
	uint32_t workerId = 0;
	StringView name;
	bool managed = false;
	bool detouched = false;
};

/* Interface for thread workers or handlers */
template <typename Interface>
class ThreadInterface : public RefBase<Interface> {
public:
	virtual ~ThreadInterface() { }

	static const void *getOwner();
	static void workerThread(ThreadInterface *tm, const void *owner);

	virtual void threadInit() { }
	virtual void threadDispose() { }
	virtual bool worker() { return false; }
};

}

#endif /* MODULES_THREADS_SPTHREAD_H_ */
