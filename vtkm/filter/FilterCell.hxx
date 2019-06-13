//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

namespace vtkm
{
namespace filter
{

//----------------------------------------------------------------------------
template <class Derived>
inline VTKM_CONT FilterCell<Derived>::FilterCell()
  : vtkm::filter::FilterField<Derived>()
  , CellSetIndex(0)
{
}

//----------------------------------------------------------------------------
template <class Derived>
inline VTKM_CONT FilterCell<Derived>::~FilterCell()
{
}
}
}
