//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DecomposerMultiBlock_h
#define vtk_m_cont_DecomposerMultiBlock_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/internal/ExportMacros.h>
#include <vtkm/thirdparty/diy/Configure.h>

// clang-format off
#include VTKM_DIY(diy/assigner.hpp)
// clang-format on

namespace vtkm
{
namespace cont
{

/// \brief DIY Decomposer that uses `MultiBlock` existing decomposition.
///
/// To create partners for various reduce operations, DIY requires a decomposer.
/// This class provides an implementation that can use the multiblock's
/// decomposition.
///
class VTKM_CONT_EXPORT DecomposerMultiBlock
{
public:
  VTKM_CONT DecomposerMultiBlock(const diy::Assigner& assigner)
    : divisions{ assigner.nblocks() }
  {
  }

  using DivisionVector = std::vector<int>;

  /// this public member is needed to satisfy decomposer concept for
  /// partners in DIY.
  DivisionVector divisions;
};
}
}

#endif
