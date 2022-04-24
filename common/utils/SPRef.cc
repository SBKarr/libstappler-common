/**
Copyright (c) 2017-2022 Roman Katuntsev <sbkarr@stappler.org>

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
#include "SPRef.h"
#include "SPLog.h"
#include "SPTime.h"

#ifdef LINUX

#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>

#ifdef MODULE_COMMON_FILESYSTEM
#include "SPFilesystem.h"
#endif

namespace stappler::memleak {

static constexpr int LinuxBacktraceSize = 128;
static constexpr int LinuxBacktraceOffset = 2;

static std::string filterBacktracePath(StringView path) {
#ifdef MODULE_COMMON_FILESYSTEM
	return filepath::replace<memory::StandartInterface>(path, filesystem::currentDir<memory::StandartInterface>(), "/");
#else
	return path.str<memory::StandartInterface>();
#endif
}

static std::vector<std::string> getBacktrace() {
	std::vector<std::string> ret;
    void *bt[LinuxBacktraceSize + LinuxBacktraceOffset];
    char **bt_syms;
    int bt_size;

    bt_size = backtrace(bt, LinuxBacktraceSize + LinuxBacktraceOffset);
    bt_syms = backtrace_symbols(bt, bt_size);
    ret.reserve(bt_size - LinuxBacktraceOffset);

    for (int i = LinuxBacktraceOffset; i < bt_size; i++) {
    	auto str = filterBacktracePath(bt_syms[i]);
    	auto first = str.find('(');
    	auto second = str.rfind('+');
    	str = str.substr(first + 1, second - first - 1);

    	int status = 0;
    	auto ptr = abi::__cxa_demangle (str.data(), nullptr, nullptr, &status);

    	if (ptr) {
    		ret.emplace_back(std::string(ptr));
        	free(ptr);
    	} else {
    		ret.emplace_back(std::string(bt_syms[i]));
    	}
    }

    free(bt_syms);
    return ret;
}

}

#else

namespace stappler::memleak {
static Vector<String> getBacktrace() {
	return Vector<String>();
}
}

#endif

namespace stappler::memleak {

static std::mutex s_mutex;
static std::atomic<uint64_t> s_refId = 1;

struct BackraceInfo {
	Time t;
	std::vector<std::string> backtrace;
};

static std::map<const RefBase<memory::StandartInterface> *, std::map<uint64_t, BackraceInfo>> s_retainStdMap;
static std::map<const RefBase<memory::PoolInterface> *, std::map<uint64_t, BackraceInfo>> s_retainPoolMap;

uint64_t getNextRefId() {
	return s_refId.fetch_add(1);
}

uint64_t retainBacktrace(const RefBase<memory::StandartInterface> *ptr) {
	auto id = getNextRefId();
	auto bt = getBacktrace();
	s_mutex.lock();

	auto it = s_retainStdMap.find(ptr);
	if (it == s_retainStdMap.end()) {
		it = s_retainStdMap.emplace(ptr, std::map<uint64_t, BackraceInfo>()).first;
	}

	auto iit = it->second.find(id);
	if (iit == it->second.end()) {
		it->second.emplace(id, BackraceInfo{Time::now(), move(bt)});
	}

	s_mutex.unlock();
	return id;
}

void releaseBacktrace(const RefBase<memory::StandartInterface> *ptr, uint64_t id) {
	if (!id) {
		return;
	}

	s_mutex.lock();

	auto it = s_retainStdMap.find(ptr);
	if (it != s_retainStdMap.end()) {
		auto iit = it->second.find(id);
		if (iit != it->second.end()) {
			it->second.erase(iit);
		}
		if (it->second.size() == 0) {
			s_retainStdMap.erase(it);
		}
	}

	s_mutex.unlock();
}

void foreachBacktrace(const RefBase<memory::StandartInterface> *ptr,
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) {
	s_mutex.lock();

	auto it = s_retainStdMap.find(ptr);
	if (it != s_retainStdMap.end()) {
		for (auto &iit : it->second) {
			cb(iit.first, iit.second.t, iit.second.backtrace);
		}
	}

	s_mutex.unlock();
}

uint64_t retainBacktrace(const RefBase<memory::PoolInterface> *ptr) {
	auto id = getNextRefId();
	auto bt = getBacktrace();
	s_mutex.lock();

	auto it = s_retainPoolMap.find(ptr);
	if (it == s_retainPoolMap.end()) {
		it = s_retainPoolMap.emplace(ptr, std::map<uint64_t, BackraceInfo>()).first;
	}

	auto iit = it->second.find(id);
	if (iit == it->second.end()) {
		it->second.emplace(id, BackraceInfo{Time::now(), move(bt)});
	}

	s_mutex.unlock();
	return id;
}

void releaseBacktrace(const RefBase<memory::PoolInterface> *ptr, uint64_t id) {
	if (!id) {
		return;
	}

	s_mutex.lock();

	auto it = s_retainPoolMap.find(ptr);
	if (it != s_retainPoolMap.end()) {
		auto iit = it->second.find(id);
		if (iit != it->second.end()) {
			it->second.erase(iit);
		}
		if (it->second.size() == 0) {
			s_retainPoolMap.erase(it);
		}
	}

	s_mutex.unlock();
}

void foreachBacktrace(const RefBase<memory::PoolInterface> *ptr,
		const Callback<void(uint64_t, Time, const std::vector<std::string> &)> &cb) {
	s_mutex.lock();

	auto it = s_retainPoolMap.find(ptr);
	if (it != s_retainPoolMap.end()) {
		for (auto &iit : it->second) {
			cb(iit.first, iit.second.t, iit.second.backtrace);
		}
	}

	s_mutex.unlock();
}

}
