//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef vtk_m_filter_scalar_topology_internal_ExtractTopVolumeContoursBlock_h
#define vtk_m_filter_scalar_topology_internal_ExtractTopVolumeContoursBlock_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/filter/scalar_topology/worklet/select_top_volume_branches/TopVolumeBranchData.h>

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{
namespace internal
{

struct ExtractTopVolumeContoursBlock
{
  ExtractTopVolumeContoursBlock(vtkm::Id localBlockNo, int globalBlockId);

  // Block metadata
  vtkm::Id LocalBlockNo;
  int GlobalBlockId;

  // The data class for branch arrays (e.g., branch root global regular IDs, branch volume, etc.)
  // we reuse the class TopVolumeBranchData that was used in SelectTopVolumeBranchesFilter,
  // because we need more than half of the arrays in this class for contour extraction
  TopVolumeBranchData TopVolumeData;

  // Isosurface output
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> IsosurfaceEdgesFrom;
  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> IsosurfaceEdgesTo;
  vtkm::worklet::contourtree_augmented::IdArrayType IsosurfaceEdgesOffset;
  vtkm::worklet::contourtree_augmented::IdArrayType IsosurfaceEdgesLabels;
  vtkm::worklet::contourtree_augmented::IdArrayType IsosurfaceEdgesOrders;
  vtkm::cont::UnknownArrayHandle IsosurfaceIsoValue;
  // this is a tricky one - it is a part of the isovalue but solely used for simulation of simplicity
  vtkm::worklet::contourtree_augmented::IdArrayType IsosurfaceGRIds;

  // Destroy function allowing DIY to own blocks and clean them up after use
  static void Destroy(void* b) { delete static_cast<ExtractTopVolumeContoursBlock*>(b); }

  // extract isosurfaces on top branches by volume
  void ExtractIsosurfaceOnSelectedBranch(const vtkm::cont::DataSet& bdDataSet,
                                         const bool isMarchingCubes,
                                         const bool shiftIsovalueByEpsilon,
                                         const vtkm::cont::LogLevel timingsLogLevel);
};

} // namespace internal
} // namespace scalar_topology
} // namespace filter
} // namespace vtkm
#endif
