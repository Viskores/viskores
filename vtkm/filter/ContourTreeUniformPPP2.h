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


#ifndef vtk_m_filter_ContourTreeUniformPPP2_h
#define vtk_m_filter_ContourTreeUniformPPP2_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/contourtree_ppp2/ContourTree.h>
#include <vtkm/worklet/contourtree_ppp2/Types.h>

#include <utility>
#include <vector>
#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{


class ContourTreePPP2 : public vtkm::filter::FilterField<ContourTreePPP2>
{
public:
  VTKM_CONT
  ContourTreePPP2(bool useMarchingCubes = false,
                  bool computeRegularStructure = true,
                  vtkm::Id cellSetId = 0);

  // Output field "saddlePeak" which is pairs of vertex ids indicating saddle and peak of contour
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

  const vtkm::worklet::contourtree_ppp2::ContourTree& GetContourTree() const;
  const vtkm::worklet::contourtree_ppp2::IdArrayType& GetSortOrder() const;
  vtkm::Id GetNumIterations() const;
  const std::vector<std::pair<std::string, vtkm::Float64>>& GetTimings() const;

private:
  bool mUseMarchingCubes;
  bool mComputeRegularStructure;
  vtkm::Id mCellSetId;
  std::vector<std::pair<std::string, vtkm::Float64>> mTimings;

  // TODO Should the additional fields below be addd to the vtkm::filter::ResultField and what is the best way to represent them
  // Additional result fields not included in the vtkm::filter::ResultField returned by DoExecute
  vtkm::worklet::contourtree_ppp2::ContourTree mContourTree; // The contour tree
  vtkm::Id nIterations; // Number of iterations used to compute the contour tree
  vtkm::worklet::contourtree_ppp2::IdArrayType
    mSortOrder; // Array with the sorted order of the mesh vertices
};

template <>
class FilterTraits<ContourTreePPP2>
{
public:
  typedef TypeListTagScalarAll InputFieldTypeList;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ContourTreeUniformPPP2.hxx>

#endif // vtk_m_filter_ContourTreeUniformPPP2_h
