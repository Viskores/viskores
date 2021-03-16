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

#ifndef vtk_m_worklet_contourtree_augmented_contourtree_mesh_inc_copy_into_combined_array_worklet_h
#define vtk_m_worklet_contourtree_augmented_contourtree_mesh_inc_copy_into_combined_array_worklet_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_contourtree_mesh_inc
{

class CopyIntoCombinedArrayWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn thisArray,
                                WholeArrayIn otherArray,
                                ExecObject comparisonFunctor,
                                WholeArrayOut resultArray);
  typedef void ExecutionSignature(_1, InputIndex, _2, _3, _4);
  typedef _1 InputDomain;

  template <typename InputType,
            typename InputArrayPortalType,
            typename ComparisonFunctorType,
            typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const InputType& value,
                            vtkm::Id idx,
                            const InputArrayPortalType& otherArrayPortal,
                            const ComparisonFunctorType& comparisonFunctor,
                            OutputArrayPortalType& resultArrayPortal) const
  {
    //std::cout << "Value " << value << " at idx " << idx << ": ";
    // Binary search to find position of our value in other array
    vtkm::Id l = 0;
    vtkm::Id r = otherArrayPortal.GetNumberOfValues();

    while (r > l)
    {
      vtkm::Id pos = (l + r) / 2;
      if (comparisonFunctor(value, otherArrayPortal.Get(pos)))
      {
        r = pos - 1;
      }
      else if (comparisonFunctor(otherArrayPortal.Get(pos), value))
      {
        l = pos + 1;
      }
      else
      {
        l = r = pos;
      }
    }

    // l (and r) are position of element in other array with same value as other if
    // such a value exists. Otherwise either the element in the other array before
    // or after. Adapt position as necessary.
    // Note: Shift positions with equal value (must have equal global mesh index due
    // to simulation of simplicity) one up for one array (this Array) so that
    // elements that occur in both arrays do not overwrite each other.
    vtkm::Id posInOther = l;
    //std::cout << "l=" << l << " r=" << r;
    if (posInOther < otherArrayPortal.GetNumberOfValues())
    {
      //std::cout << " [ " << comparisonFunctor(otherArrayPortal.Get(posInOther), value) << " " << vtkm::worklet::contourtree_augmented::IsThis(value) << " " << comparisonFunctor.GetGlobalMeshIndex(otherArrayPortal.Get(posInOther)) << " " << comparisonFunctor.GetGlobalMeshIndex(value) << " ]";
      if (comparisonFunctor(otherArrayPortal.Get(posInOther), value) ||
          (vtkm::worklet::contourtree_augmented::IsThis(value) &&
           comparisonFunctor.GetGlobalMeshIndex(otherArrayPortal.Get(posInOther)) ==
             comparisonFunctor.GetGlobalMeshIndex(value)))
      {
        ++posInOther;
      }
    }

    //std::cout << " posInOther=" << posInOther;
    // The position of the current elemnt is its index in our array plus its
    // position in the ohter array.
    resultArrayPortal.Set(idx + posInOther, value);
    //std::cout << " -> Setting " << idx + posInOther << " to " << value << std::endl;
  }
}; //  CopyIntoCombinedArrayWorklet


} // namespace mesh_dem_contourtree_mesh_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm

#endif
