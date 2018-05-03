//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_VirtualObjectBase_h
#define vtk_m_VirtualObjectBase_h

#include <vtkm/Types.h>

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
class VTKM_ALWAYS_EXPORT VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual ~VirtualObjectBase(){
    //we implement this as we need a destructor with cuda markup
    //but using =default causes warnings with CUDA 9
  };

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

#endif //vtk_m_VirtualObjectBase_h
