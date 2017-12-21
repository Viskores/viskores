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
#ifndef vtk_m_cont_AssignerMultiBlock_h
#define vtk_m_cont_AssignerMultiBlock_h

#include <vtkm/internal/Configure.h>
#if defined(VTKM_ENABLE_MPI)

#include <diy/assigner.hpp>
#include <vtkm/cont/MultiBlock.h>

namespace vtkm
{
namespace cont
{

/// \brief Assigner for `MultiBlock` blocks.
///
/// `AssignerMultiBlock` is a `diy::Assigner` implementation that uses
/// `MultiBlock`'s block distribution to build global-id/rank associations
/// needed for several `diy` operations.
/// It uses a contiguous assignment strategy to map blocks to global ids i.e.
/// blocks on rank 0 come first, then rank 1, etc. Any rank may have 0 blocks.
///
/// AssignerMultiBlock uses collectives in the constructor hence it is
/// essential it gets created on all ranks irrespective of whether the rank has
/// any blocks.
///
class VTKM_CONT_EXPORT AssignerMultiBlock : public diy::Assigner
{
public:
  /// Initialize the assigner using a multiblock dataset.
  /// This may initialize collective operations to populate the assigner with
  /// information about blocks on all ranks.
  VTKM_CONT
  AssignerMultiBlock(const vtkm::cont::MultiBlock& mb);

  ///@{
  /// diy::Assigner API implementation.
  VTKM_CONT
  void local_gids(int rank, std::vector<int>& gids) const override;

  VTKM_CONT
  int rank(int gid) const override;
  //@}
private:
  std::vector<vtkm::Id> IScanBlockCounts;
};
}
}

#endif // defined(VTKM_ENABLE_MPI)
#endif
