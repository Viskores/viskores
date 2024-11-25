//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/PointLocatorBase.h>

namespace vtkm
{
namespace cont
{

void PointLocatorBase::Update() const
{
  if (this->Modified)
  {
    // Although the data of the derived class may change, the logical state
    // of the class should not. Thus, we will instruct the compiler to relax
    // the const constraint.
    const_cast<PointLocatorBase*>(this)->Build();
    this->Modified = false;
  }
}

}
} // namespace vtkm::cont
