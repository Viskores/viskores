//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Token.h>

#include <list>

using LockType = std::unique_lock<std::mutex>;

class vtkm::cont::Token::InternalStruct
{
  std::mutex Mutex;
  std::list<vtkm::cont::Token::HeldReference> HeldReferences;

  VTKM_CONT void CheckLock(const LockType& lock) const
  {
    VTKM_ASSERT((lock.mutex() == &this->Mutex) && (lock.owns_lock()));
  }

public:
  LockType GetLock() { return LockType(this->Mutex); }
  std::list<vtkm::cont::Token::HeldReference>* GetHeldReferences(const LockType& lock)
  {
    this->CheckLock(lock);
    return &this->HeldReferences;
  }
};

struct vtkm::cont::Token::HeldReference
{
  std::unique_ptr<vtkm::cont::Token::ObjectReference> ObjectReference;
  vtkm::cont::Token::ReferenceCount* ReferenceCountPointer;
  std::mutex* MutexPointer;
  std::condition_variable* ConditionVariablePointer;

  HeldReference(std::unique_ptr<vtkm::cont::Token::ObjectReference>&& objRef,
                vtkm::cont::Token::ReferenceCount* refCountP,
                std::mutex* mutexP,
                std::condition_variable* conditionVariableP)
    : ObjectReference(std::move(objRef))
    , ReferenceCountPointer(refCountP)
    , MutexPointer(mutexP)
    , ConditionVariablePointer(conditionVariableP)
  {
  }
};

vtkm::cont::Token::Token() {}

vtkm::cont::Token::Token(Token&& rhs)
  : Internals(std::move(rhs.Internals))
{
}

vtkm::cont::Token::~Token()
{
  this->DetachFromAll();
}

void vtkm::cont::Token::DetachFromAll()
{
  if (!this->Internals)
  {
    // If internals is NULL, then we are not attached to anything.
    return;
  }
  LockType localLock = this->Internals->GetLock();
  auto heldReferences = this->Internals->GetHeldReferences(localLock);
  for (auto&& held : *heldReferences)
  {
    LockType objectLock(*held.MutexPointer);
    *held.ReferenceCountPointer -= 1;
    objectLock.unlock();
    held.ConditionVariablePointer->notify_all();
  }
  heldReferences->clear();
}

vtkm::cont::Token::Reference vtkm::cont::Token::GetReference() const
{
  if (!this->Internals)
  {
    this->Internals.reset(new InternalStruct);
  }

  return this->Internals.get();
}

void vtkm::cont::Token::Attach(std::unique_ptr<vtkm::cont::Token::ObjectReference>&& objectRef,
                               vtkm::cont::Token::ReferenceCount* referenceCountPointer,
                               std::unique_lock<std::mutex>& lock,
                               std::condition_variable* conditionVariablePointer)
{
  if (!this->Internals)
  {
    this->Internals.reset(new InternalStruct);
  }
  LockType localLock = this->Internals->GetLock();
  if (this->IsAttached(localLock, referenceCountPointer))
  {
    // Already attached.
    return;
  }
  if (!lock.owns_lock())
  {
    lock.lock();
  }
  *referenceCountPointer += 1;
  this->Internals->GetHeldReferences(localLock)->emplace_back(
    std::move(objectRef), referenceCountPointer, lock.mutex(), conditionVariablePointer);
}

inline bool vtkm::cont::Token::IsAttached(
  LockType& lock,
  vtkm::cont::Token::ReferenceCount* referenceCountPointer) const
{
  if (!this->Internals)
  {
    return false;
  }
  for (auto&& heldReference : *this->Internals->GetHeldReferences(lock))
  {
    if (referenceCountPointer == heldReference.ReferenceCountPointer)
    {
      return true;
    }
  }
  return false;
}

bool vtkm::cont::Token::IsAttached(vtkm::cont::Token::ReferenceCount* referenceCountPointer) const
{
  if (!this->Internals)
  {
    return false;
  }
  LockType lock = this->Internals->GetLock();
  return this->IsAttached(lock, referenceCountPointer);
}
