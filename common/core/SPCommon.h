/**
Copyright (c) 2016-2022 Roman Katuntsev <sbkarr@stappler.org>

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

#ifndef COMMON_CORE_SPCOMMON_H_
#define COMMON_CORE_SPCOMMON_H_

#include "SPCore.h"
#include "SPMemInterface.h"
#include "SPMemUuid.h"

namespace stappler {

void getBacktrace(size_t offset, const Callback<void(StringView)> &);

}

#if LINUX
#if DEBUG

#define TEXTIFY(A) #A

#define SP_DEFINE_GDB_SCRIPT(path, script_name) \
  asm("\
.pushsection \".debug_gdb_scripts\", \"MS\",@progbits,1\n\
.byte 1\n\
.asciz \"" TEXTIFY(path) script_name "\"\n\
.popsection \n\
");

SP_DEFINE_GDB_SCRIPT(STAPPLER_ROOT, "/gdb/printers.py")

#endif
#endif

#endif /* COMMON_CORE_SPCOMMON_H_ */
