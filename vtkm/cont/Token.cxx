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

vtkm::cont::Token::Token()
  : Internals(new InternalStruct)
{
}

vtkm::cont::Token::~Token()
{
  this->DetachFromAll();
}

void vtkm::cont::Token::DetachFromAll()
{
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

void vtkm::cont::Token::Attach(std::unique_ptr<vtkm::cont::Token::ObjectReference>&& objectRef,
                               vtkm::cont::Token::ReferenceCount* referenceCountPointer,
                               std::mutex* mutexPointer,
                               std::condition_variable* conditionVariablePointer)
{
  LockType localLock = this->Internals->GetLock();
  LockType objectLock(*mutexPointer);
  *referenceCountPointer += 1;
  this->Internals->GetHeldReferences(localLock)->emplace_back(
    std::move(objectRef), referenceCountPointer, mutexPointer, conditionVariablePointer);
}
