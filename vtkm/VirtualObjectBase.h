//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_VirtualObjectBase_h
#define vtk_m_VirtualObjectBase_h

#include <vtkm/Deprecated.h>
#include <vtkm/Types.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
// Do not include this class at all if not compiling virtual methods.

// This is a deprecated class. Don't warn about deprecation while implementing
// deprecated functionality.
VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{

/// \brief Base class for virtual objects that work in the execution environment
///
/// Any class built in VTK-m that has virtual methods and is intended to work in both the control
/// and execution environment should inherit from \c VirtualObjectBase. Hierarchies under \c
/// VirtualObjectBase can be used in conjunction with \c VirtualObjectHandle to transfer from the
/// control environment (where they are set up) to the execution environment (where they are used).
///
/// In addition to inheriting from \c VirtualObjectBase, virtual objects have to satisfy 2 other
/// conditions to work correctly. First, they have to be a plain old data type that can be copied
/// with \c memcpy (with the exception of the virtual table, which \c VirtualObjectHandle will take
/// care of). Second, if the object changes its state in the control environment, it should call
/// \c Modified on itself so the \c VirtualObjectHandle will know it update the object in the
/// execution environment.
///
class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.7,
  "Virtual methods are no longer supported in the execution environment.") VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual ~VirtualObjectBase() noexcept
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC_CONT void Modified() { this->ModifiedCount++; }

  VTKM_EXEC_CONT vtkm::Id GetModifiedCount() const { return this->ModifiedCount; }

protected:
  VTKM_EXEC_CONT VirtualObjectBase()
    : ModifiedCount(0)
  {
  }

  VTKM_EXEC_CONT VirtualObjectBase(const VirtualObjectBase& other)
  { //we implement this as we need a copy constructor with cuda markup
    //but using =default causes warnings with CUDA 9
    this->ModifiedCount = other.ModifiedCount;
  }

  VTKM_EXEC_CONT VirtualObjectBase(VirtualObjectBase&& other)
    : ModifiedCount(other.ModifiedCount)
  {
  }

  VTKM_EXEC_CONT VirtualObjectBase& operator=(const VirtualObjectBase&)
  {
    this->Modified();
    return *this;
  }

  VTKM_EXEC_CONT VirtualObjectBase& operator=(VirtualObjectBase&&)
  {
    this->Modified();
    return *this;
  }

private:
  vtkm::Id ModifiedCount;
};

} // namespace vtkm

VTKM_DEPRECATED_SUPPRESS_END

#endif //VTKM_NO_DEPRECATED_VIRTUAL

#endif //vtk_m_VirtualObjectBase_h
