/* Copyright (c) 2006, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Sanjay Ghemawat
 */

// Implementation of atomic operations for x86.  This file should not
// be included directly.  Clients should instead include
// "base/atomicops.h".

#ifndef BASE_ATOMICOPS_INTERNALS_X86_H__
#define BASE_ATOMICOPS_INTERNALS_X86_H__

typedef intptr_t AtomicWord;

// There are a couple places we need to specialize opcodes to account for the
// different AtomicWord sizes on x86_64 and 32-bit platforms.
// This macro is undefined after its last use, below.
#if defined(__x86_64__)
#define ATOMICOPS_WORD_SUFFIX "q"
#else
#define ATOMICOPS_WORD_SUFFIX "l"
#endif

// This struct is not part of the public API of this module; clients may not
// use it.
// Features of this x86.  Values may not be correct before main() is run,
// but are set conservatively.
struct AtomicOps_x86CPUFeatureStruct {
  bool has_amd_lock_mb_bug; // Processor has AMD memory-barrier bug; do lfence
                            // after acquire compare-and-swap.
  bool has_sse2;            // Processor has SSE2.
  bool has_cmpxchg16b;      // Processor supports cmpxchg16b instruction.
};
extern struct AtomicOps_x86CPUFeatureStruct AtomicOps_Internalx86CPUFeatures;

inline AtomicWord CompareAndSwap(volatile AtomicWord* ptr,
                                 AtomicWord old_value,
                                 AtomicWord new_value) {
  AtomicWord prev;
  __asm__ __volatile__("lock; cmpxchg" ATOMICOPS_WORD_SUFFIX " %1,%2"
                       : "=a" (prev)
                       : "q" (new_value), "m" (*ptr), "0" (old_value)
                       : "memory");
  return prev;
}

inline AtomicWord AtomicExchange(volatile AtomicWord* ptr,
                                 AtomicWord new_value) {
  __asm__ __volatile__("xchg" ATOMICOPS_WORD_SUFFIX " %1,%0" // The lock prefix
                       : "=r" (new_value)                    // is implicit for
                       : "m" (*ptr), "0" (new_value)         // xchg.
                       : "memory");
  return new_value;  // Now it's the previous value.
}

inline AtomicWord AtomicIncrement(volatile AtomicWord* ptr, AtomicWord increment) {
  AtomicWord temp = increment;
  __asm__ __volatile__("lock; xadd" ATOMICOPS_WORD_SUFFIX " %0,%1"
                       : "+r" (temp), "+m" (*ptr)
                       : : "memory");
  // temp now contains the previous value of *ptr
  return temp + increment;
}

#undef ATOMICOPS_WORD_SUFFIX


inline AtomicWord Acquire_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  AtomicWord x = CompareAndSwap(ptr, old_value, new_value);
  if (AtomicOps_Internalx86CPUFeatures.has_amd_lock_mb_bug) {
    __asm__ __volatile__("lfence" : : : "memory");
  }
  return x;
}

inline AtomicWord Release_CompareAndSwap(volatile AtomicWord* ptr,
                                         AtomicWord old_value,
                                         AtomicWord new_value) {
  return CompareAndSwap(ptr, old_value, new_value);
}

#define ATOMICOPS_COMPILER_BARRIER() __asm__ __volatile__("" : : : "memory")

#if defined(__x86_64__)

inline void MemoryBarrier() {
  __asm__ __volatile__("mfence" : : : "memory");
}

inline void Acquire_Store(volatile AtomicWord* ptr, AtomicWord value) {
  *ptr = value;
  MemoryBarrier();
}

#else

inline void MemoryBarrier() {
  if (AtomicOps_Internalx86CPUFeatures.has_sse2) {
    __asm__ __volatile__("mfence" : : : "memory");
  } else { // mfence is faster but not present on PIII
    AtomicWord x = 0;
    AtomicExchange(&x, 0);
  }
}

inline void Acquire_Store(volatile AtomicWord* ptr, AtomicWord value) {
  if (AtomicOps_Internalx86CPUFeatures.has_sse2) {
    *ptr = value;
    __asm__ __volatile__("mfence" : : : "memory");
  } else {
    AtomicExchange(ptr, value);
  }
}

#endif

inline void Release_Store(volatile AtomicWord* ptr, AtomicWord value) {
  ATOMICOPS_COMPILER_BARRIER();
  *ptr = value; // works w/o barrier for current Intel chips as of June 2005

  // When new chips come out, check:
  //  IA-32 Intel Architecture Software Developer's Manual, Volume 3:
  //  System Programming Guide, Chatper 7: Multiple-processor management,
  //  Section 7.2, Memory Ordering.
  // Last seen at:
  //   http://developer.intel.com/design/pentium4/manuals/index_new.htm
}

inline AtomicWord Acquire_Load(volatile const AtomicWord* ptr) {
  AtomicWord value = *ptr;
  MemoryBarrier();
  return value;
}

inline AtomicWord Release_Load(volatile const AtomicWord* ptr) {
  MemoryBarrier();
  return *ptr;
}

#undef ATOMICOPS_COMPILER_BARRIER

#endif  // BASE_ATOMICOPS_INTERNALS_X86_H__
