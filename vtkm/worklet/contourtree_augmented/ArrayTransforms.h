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

// include guard
#ifndef vtkm_worklet_contourtree_augmented_array_transforms_h
#define vtkm_worklet_contourtree_augmented_array_transforms_h

// global libraries
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>

// local includes
#include <vtkm/worklet/contourtree_augmented/Types.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{


// permute routines
template <typename ValueType, typename ArrayType>
void permuteArray(const ArrayType& input, IdArrayType& permute, ArrayType& output)
{ // permuteValues()
  using transform_type =
    vtkm::cont::ArrayHandleTransform<IdArrayType, MaskedIndexFunctor<ValueType>>;
  using permute_type = vtkm::cont::ArrayHandlePermutation<transform_type, ArrayType>;

  // resize the output, i.e., output.resize(permute.size());
  vtkm::Id permNumValues = permute.GetNumberOfValues();
  vtkm::Id outNumValues = output.GetNumberOfValues();
  if (permNumValues > outNumValues)
  {
    output.Allocate(permNumValues);
  }
  else if (permNumValues < outNumValues)
  {
    output.Shrink(permNumValues);
  } // else the output has already the correct size

  // The following is equivalent to doing the following in serial
  //
  // for (vtkm::Id entry = 0; entry < permute.size(); entry++)
  //     output[entry] = input[maskedIndex(permute[entry])];
  //
  // Apply the maskedIndex functor to the permute array. ArrayHandleTransform is a fancy vtkm array, i.e.,
  // the function is applied on-the-fly without creating a copy of the array.
  transform_type maskedPermuteIndex =
    vtkm::cont::make_ArrayHandleTransform(permute, MaskedIndexFunctor<ValueType>());
  // Permute the input array based on the maskedPermuteIndex array. Again, ArrayHandlePermutation is a
  // fancy vtkm array so that we do not actually copy any data here
  permute_type permutedInput(maskedPermuteIndex, input);
  // Finally, copy the permuted values to the output array
  vtkm::cont::ArrayCopy(permutedInput, output);
} // permuteValues()

// permuteValues from the original PPP2 code is equivalent to permuteArray<T, vtkm::cont:ArrayHandle<T, StorageType>, DeviceAdaptor>   and has therefore been removed
/*template <typename T, typename StorageType, typename DeviceAdapter>
void permuteValues(const vtkm::cont::ArrayHandle<T,StorageType> &input, IdArrayType &permute, vtkm::cont::ArrayHandle<T,StorageType> &output)
{}
*/

// permuteIndicies from the original PPP2 code is equivalent to permuteArray<vtkm::Id, IdArrayType, DeviceAdaptor> and has therefore been removed
/* template <typename DeviceAdapter>
void permuteIndices(IdArrayType &input, IdArrayType &permute, IdArrayType &output)
{}
*/

// tranform functor needed for ScanExclusive calculation. Return 0 if noSuchElement else 1
struct oneIfArcValid
{
  VTKM_EXEC_CONT
  oneIfArcValid() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id a) const { return noSuchElement(a) ? 0 : 1; }
};

// transform functor used in ContourTreeMesh to flag indicies as other when using the CombinedVectorClass
struct markOther
{
  VTKM_EXEC_CONT
  markOther() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id idx) const { return idx | CV_OTHER_FLAG; }
};

// transform functor needed for ScanExclusive calculation. Return 1 if vertex is critical else 0.
struct onefIfCritical
{
  VTKM_EXEC_CONT
  onefIfCritical() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id x) const { return x != 1 ? 1 : 0; }
};

// transform functor needed for ScanExclusive calculation in FindSuperAndHyperNodes. Return 1 if vertex is a supernode, else 0.
struct onefIfSupernode
{
  VTKM_EXEC_CONT
  onefIfSupernode() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id x) const { return isSupernode(x) ? 1 : 0; }
};

// transform functor needed for ScanExclusive calculation in FindSuperAndHyperNodes. Return 1 if vertex is a hypernode, else 0.
struct onefIfHypernode
{
  VTKM_EXEC_CONT
  onefIfHypernode() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(vtkm::Id x) const { return isHypernode(x) ? 1 : 0; }
};


} // namespace contourtree_augmented
} // worklet
} // vtkm

// tail of include guard
#endif
