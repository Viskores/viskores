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


#ifndef vtk_m_filter_ContourTreeUniformAugmented_h
#define vtk_m_filter_ContourTreeUniformAugmented_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

#include <utility>
#include <vector>
#include <vtkm/Bounds.h>
#include <vtkm/filter/FilterCell.h>

namespace vtkm
{
namespace filter
{

namespace detail
{
class MultiBlockContourTreeHelper;
} // namespace detail


class ContourTreePPP2 : public vtkm::filter::FilterCell<ContourTreePPP2>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;
  VTKM_CONT
  ContourTreePPP2(bool useMarchingCubes = false, unsigned int computeRegularStructure = 1);

  // Define the spatial decomposition of the data in case we run in parallel with a multi-block dataset
  VTKM_CONT
  void SetSpatialDecomposition(vtkm::Id3 blocksPerDim,
                               vtkm::Id3 globalSize,
                               const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
                               const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
                               const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes);

  // Output field "saddlePeak" which is pairs of vertex ids indicating saddle and peak of contour
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //@{
  /// when operating on vtkm::cont::MultiBlock we want to
  /// do processing across ranks as well. Just adding pre/post handles
  /// for the same does the trick.
  template <typename DerivedPolicy>
  VTKM_CONT void PreExecute(const vtkm::cont::PartitionedDataSet& input,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT void PostExecute(const vtkm::cont::PartitionedDataSet& input,
                             vtkm::cont::PartitionedDataSet& output,
                             const vtkm::filter::PolicyBase<DerivedPolicy>&);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT void DoPostExecute(
    const vtkm::cont::PartitionedDataSet& input,
    vtkm::cont::PartitionedDataSet& output,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::cont::ArrayHandle<T, StorageType>&, // dummy parameter to get the type
    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  const vtkm::worklet::contourtree_augmented::ContourTree& GetContourTree() const;
  const vtkm::worklet::contourtree_augmented::IdArrayType& GetSortOrder() const;
  vtkm::Id GetNumIterations() const;
  const std::vector<std::pair<std::string, vtkm::Float64>>& GetTimings() const;

private:
  // Given the input dataset determine the number of rows, cols, and slices
  void getDims(const vtkm::cont::DataSet& input,
               vtkm::Id& nrows,
               vtkm::Id& ncols,
               vtkm::Id nslices) const;

  bool UseMarchingCubes;
  // 0=no augmentation, 1=full augmentation, 2=boundary augmentation
  unsigned int ComputeRegularStructure;
  // Store timings about the contour tree computation
  std::vector<std::pair<std::string, vtkm::Float64>> Timings;

  // TODO Should the additional fields below be add to the vtkm::filter::ResultField and what is the best way to represent them
  // Additional result fields not included in the vtkm::filter::ResultField returned by DoExecute
  // The contour tree
  vtkm::worklet::contourtree_augmented::ContourTree ContourTreeData;
  // Number of iterations used to compute the contour tree
  vtkm::Id NumIterations;
  // Array with the sorted order of the mesh vertices
  vtkm::worklet::contourtree_augmented::IdArrayType MeshSortOrder;
  // Helper object to help with the parallel merge when running with DIY in parallel with MulitBlock data
  detail::MultiBlockContourTreeHelper* MultiBlockTreeHelper;
};


// Helper struct to collect sizing information from the dataset
struct GetRowsColsSlices
{
  void operator()(const vtkm::cont::CellSetStructured<2>& cells,
                  vtkm::Id& nRows,
                  vtkm::Id& nCols,
                  vtkm::Id& nSlices) const
  {
    vtkm::Id2 pointDimensions = cells.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = 1;
  }
  void operator()(const vtkm::cont::CellSetStructured<3>& cells,
                  vtkm::Id& nRows,
                  vtkm::Id& nCols,
                  vtkm::Id& nSlices) const
  {
    vtkm::Id3 pointDimensions = cells.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = pointDimensions[2];
  }
  template <typename T>
  void operator()(const T& cells, vtkm::Id& nRows, vtkm::Id& nCols, vtkm::Id& nSlices) const
  {
    (void)nRows;
    (void)nCols;
    (void)nSlices;
    (void)cells;
    throw vtkm::cont::ErrorBadValue("Expected 2D or 3D structured cell cet! ");
  }
};

} // namespace filter
} // namespace vtkm

#include <vtkm/filter/ContourTreeUniformAugmented.hxx>

#endif // vtk_m_filter_ContourTreeUniformAugmented_h
