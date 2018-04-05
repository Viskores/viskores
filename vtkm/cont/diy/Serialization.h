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
#ifndef vtk_m_cont_diy_Serialization_h
#define vtk_m_cont_diy_Serialization_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/internal/ExportMacros.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include VTKM_DIY(diy/serialization.hpp)
VTKM_THIRDPARTY_POST_INCLUDE

namespace diy
{

/// This provides specializations to extend DIY's serialization code to
/// load/save vtkm::cont::ArrayHandle instances.
template <typename T, typename StorageTag>
struct Serialization<vtkm::cont::ArrayHandle<T, StorageTag>>
{
  VTKM_CONT
  static void save(BinaryBuffer& bb, const vtkm::cont::ArrayHandle<T, StorageTag>& indata);

  VTKM_CONT
  static void load(BinaryBuffer& bb, vtkm::cont::ArrayHandle<T, StorageTag>& outdata);
};

} // namespace diy

#include <vtkm/cont/diy/Serialization.hxx>

#endif
