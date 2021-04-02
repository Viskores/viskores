//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_Token_h
#define vtk_m_cont_Token_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <type_traits>

namespace vtkm
{
namespace cont
{

/// \brief A token to hold the scope of an `ArrayHandle` or other object.
///
/// A `Token` is an object that is held in the stack or state of another object and
/// is used when creating references to resouces that may be used by other threads.
/// For example, when preparing an `ArrayHandle` or `ExecutionObject` for a device,
/// a `Token` is given. The returned object will be valid as long as the `Token`
/// remains in scope.
///
class VTKM_CONT_EXPORT Token final
{
  class InternalStruct;
  mutable std::unique_ptr<InternalStruct> Internals;

  struct HeldReference;

  struct ObjectReference
  {
    virtual ~ObjectReference() = default;
  };

  template <typename ObjectType>
  struct ObjectReferenceImpl : ObjectReference
  {
    ObjectType Object;

    ObjectReferenceImpl(const ObjectType& object)
      : Object(object)
    {
    }

    ObjectReferenceImpl(ObjectType&& object)
      : Object(std::move(object))
    {
    }

    ObjectReferenceImpl() = delete;
    ObjectReferenceImpl(const ObjectReferenceImpl&) = delete;

    ~ObjectReferenceImpl() override = default;
  };

public:
  VTKM_CONT Token();
  VTKM_CONT Token(Token&& rhs);
  VTKM_CONT ~Token();

  /// Use this type to represent counts of how many tokens are holding a resource.
  using ReferenceCount = vtkm::IdComponent;

  /// Detaches this `Token` from all resources to allow them to be used elsewhere
  /// or deleted.
  ///
  VTKM_CONT void DetachFromAll();

  ///@{
  /// \brief Add an object to attach to the `Token`.
  ///
  /// To attach an object to a `Token`, you need the object itself, a pointer to
  /// a `Token::ReferenceCount` that is used to count how many `Token`s hold the
  /// object, a pointer to a `std::mutex` used to safely use the `ReferenceCount`,
  /// and a pointer to a `std::condition_variable` that other threads will wait
  /// on if they are blocked by the `Token` (using the same `mutex` in the given
  /// `unique_lock`). The mutex can also be passed in as a
  /// `std::unique_lock<std::mutex>` to signal whether or not the mutex is already
  /// locked by the current thread.
  ///
  /// When the `Token` is attached, it will increment the reference count (safely
  /// with the mutex) and store away these items. Other items will be able tell
  /// if a token is attached to the object by looking at the reference count.
  ///
  /// When the `Token` is released, it will decrement the reference count (safely
  /// with the mutex) and then notify all threads waiting on the condition
  /// variable.
  ///
  template <typename T>
  VTKM_CONT void Attach(T&& object,
                        vtkm::cont::Token::ReferenceCount* referenceCountPointer,
                        std::unique_lock<std::mutex>& lock,
                        std::condition_variable* conditionVariablePointer)
  {
    this->Attach(std::unique_ptr<ObjectReference>(
                   new ObjectReferenceImpl<typename std::decay<T>::type>(std::forward<T>(object))),
                 referenceCountPointer,
                 lock,
                 conditionVariablePointer);
  }

  template <typename T>
  VTKM_CONT void Attach(T&& object,
                        vtkm::cont::Token::ReferenceCount* referenceCountPoiner,
                        std::mutex* mutexPointer,
                        std::condition_variable* conditionVariablePointer)
  {
    std::unique_lock<std::mutex> lock(*mutexPointer, std::defer_lock);
    this->Attach(std::forward<T>(object), referenceCountPoiner, lock, conditionVariablePointer);
  }
  ///@}

  /// \brief Determine if this `Token` is already attached to an object.
  ///
  /// Given a reference counter pointer, such as would be passed to the `Attach` method,
  /// returns true if this `Token` is already attached, `false` otherwise.
  ///
  VTKM_CONT bool IsAttached(vtkm::cont::Token::ReferenceCount* referenceCountPointer) const;

  class Reference
  {
    friend Token;
    const void* InternalsPointer;
    VTKM_CONT Reference(const void* internalsPointer)
      : InternalsPointer(internalsPointer)
    {
    }

  public:
    VTKM_CONT bool operator==(Reference rhs) const
    {
      return this->InternalsPointer == rhs.InternalsPointer;
    }
    VTKM_CONT bool operator!=(Reference rhs) const
    {
      return this->InternalsPointer != rhs.InternalsPointer;
    }
  };

  /// \brief Returns a reference object to this `Token`.
  ///
  /// `Token` objects cannot be copied and generally are not shared. However, there are cases
  /// where you need to save a reference to a `Token` belonging to someone else so that it can
  /// later be compared. Saving a pointer to a `Token` is not always safe because `Token`s can
  /// be moved. To get around this problem, you can save a `Reference` to the `Token`. You
  /// cannot use the `Reference` to manipulate the `Token` in any way (because you do not
  /// own it). Rather, a `Reference` can just be used to compare to a `Token` object (or another
  /// `Reference`).
  ///
  VTKM_CONT Reference GetReference() const;

private:
  VTKM_CONT void Attach(std::unique_ptr<vtkm::cont::Token::ObjectReference>&& objectReference,
                        vtkm::cont::Token::ReferenceCount* referenceCountPointer,
                        std::unique_lock<std::mutex>& lock,
                        std::condition_variable* conditionVariablePointer);

  VTKM_CONT bool IsAttached(std::unique_lock<std::mutex>& lock,
                            vtkm::cont::Token::ReferenceCount* referenceCountPointer) const;
};

VTKM_CONT inline bool operator==(const vtkm::cont::Token& token, vtkm::cont::Token::Reference ref)
{
  return token.GetReference() == ref;
}
VTKM_CONT inline bool operator!=(const vtkm::cont::Token& token, vtkm::cont::Token::Reference ref)
{
  return token.GetReference() != ref;
}

VTKM_CONT inline bool operator==(vtkm::cont::Token::Reference ref, const vtkm::cont::Token& token)
{
  return ref == token.GetReference();
}
VTKM_CONT inline bool operator!=(vtkm::cont::Token::Reference ref, const vtkm::cont::Token& token)
{
  return ref != token.GetReference();
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Token_h
