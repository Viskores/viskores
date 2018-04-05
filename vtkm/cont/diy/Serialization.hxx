//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayPortalToIterators.h>

namespace diy
{

template <typename T, typename S>
inline VTKM_CONT void Serialization<vtkm::cont::ArrayHandle<T, S>>::save(
  BinaryBuffer& bb,
  const vtkm::cont::ArrayHandle<T, S>& indata)
{
  const vtkm::Id numValues = indata.GetNumberOfValues();
  diy::Serialization<vtkm::Id>::save(bb, numValues);

  auto const_portal = indata.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numValues; ++index)
  {
    diy::Serialization<T>::save(bb, const_portal.Get(index));
  }
}

template <typename T, typename S>
inline VTKM_CONT void Serialization<vtkm::cont::ArrayHandle<T, S>>::load(
  BinaryBuffer& bb,
  vtkm::cont::ArrayHandle<T, S>& outdata)
{
  vtkm::Id numValues;
  diy::Serialization<vtkm::Id>::load(bb, numValues);

  outdata.Allocate(numValues);

  T val;
  auto portal = outdata.GetPortalControl();
  for (vtkm::Id index = 0; index < numValues; ++index)
  {
    diy::Serialization<T>::load(bb, val);
    portal.Set(index, val);
  }
}
}
