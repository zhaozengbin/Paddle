// Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "paddle/fluid/memory/allocation/locked_allocator.h"
#include <mutex>  // NOLINT
#include <utility>
#include "paddle/fluid/memory/allocation/allocation_with_underlying.h"
#include "paddle/fluid/platform/lock_guard_ptr.h"
namespace paddle {
namespace memory {
namespace allocation {

bool LockedAllocator::IsAllocThreadSafe() const { return true; }

LockedAllocator::LockedAllocator(
    std::unique_ptr<Allocator> &&underlying_allocator)
    : underlying_allocator_(std::move(underlying_allocator)) {
  PADDLE_ENFORCE_NOT_NULL(underlying_allocator_);
  if (!underlying_allocator_->IsAllocThreadSafe()) {
    mtx_.reset(new std::mutex());
  }
}
void LockedAllocator::Free(Allocation *allocation) {
  {
    platform::LockGuardPtr<std::mutex> guard(mtx_);
    reinterpret_cast<AllocationWithUnderlying *>(allocation)
        ->allocation_.reset();  // Destroy inner allocation
  }
  delete allocation;
}
Allocation *LockedAllocator::AllocateImpl(size_t size, Allocator::Attr attr) {
  platform::LockGuardPtr<std::mutex> guard(mtx_);
  return new AllocationWithUnderlying(
      underlying_allocator_->Allocate(size, attr));
}
}  // namespace allocation
}  // namespace memory
}  // namespace paddle
