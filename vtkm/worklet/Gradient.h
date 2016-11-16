//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_Gradient_h
#define vtk_m_worklet_Gradient_h

#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/exec/internal/VecFromPortalPermute.h>
#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/CellTraits.h>
#include <vtkm/VecTraits.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm {
namespace worklet {

struct GradientOutTypes
    : vtkm::ListTagBase<
                        vtkm::Vec<vtkm::Float32,3>,
                        vtkm::Vec<vtkm::Float64,3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,2>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,2>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,3>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,3>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float32,4>, 3>,
                        vtkm::Vec< vtkm::Vec<vtkm::Float64,4>, 3>
                        >
{  };
struct CellGradient : vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(CellSetIn,
                                FieldInPoint<Vec3> pointCoordinates,
                                FieldInPoint<FieldCommon> inputField,
                                FieldOutCell<GradientOutTypes> outputField);

  typedef void ExecutionSignature(CellShape, PointCount, _2, _3, _4);
  typedef _1 InputDomain;

  template <typename CellTagType, typename PointCoordVecType,
    typename FieldInVecType, typename FieldOutType>
  VTKM_EXEC_EXPORT void operator()(CellTagType shape,
    vtkm::IdComponent pointCount, const PointCoordVecType& pointCoordinates,
    const FieldInVecType& inputField, FieldOutType& outputField) const
  {
    //To confirm that we have the proper input and output types we need
    //to verify that input type matches the output vtkm::Vec<T> 'T' type.
    //For example:
    // input is float => output is vtkm::Vec<float>
    // input is vtkm::Vec<float> => output is vtkm::Vec< vtkm::Vec< float > >

    //Grab the dimension tag for the input
    using InValueType = typename FieldInVecType::ComponentType;
    using InDimensionTag = typename TypeTraits<InValueType>::DimensionalityTag;

    //grab the dimension tag for the output component type
    using OutValueType = typename FieldOutType::ComponentType;
    using OutDimensionTag = typename TypeTraits<OutValueType>::DimensionalityTag;

    //Verify that input and output dimension tags match
    using Matches = typename std::is_same<InDimensionTag, OutDimensionTag>::type;

    this->Compute(shape, pointCount, pointCoordinates, inputField, outputField,
      Matches());
  }

  template <typename CellShapeTag, typename PointCoordVecType,
    typename FieldInVecType, typename FieldOutType>
  VTKM_EXEC_EXPORT void Compute(CellShapeTag shape,
    vtkm::IdComponent pointCount, const PointCoordVecType& wCoords,
    const FieldInVecType& field, FieldOutType& outputField,
    std::true_type) const
  {
    vtkm::Vec<vtkm::FloatDefault, 3> center =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, *this);
    outputField = vtkm::exec::CellDerivative(field, wCoords, center, shape, *this);
  }

  template <typename CellShapeTag,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename FieldOutType>
  VTKM_EXEC_EXPORT void Compute(CellShapeTag,
                                vtkm::IdComponent,
                                const PointCoordVecType&,
                                const FieldInVecType&,
                                FieldOutType&,
                                std::false_type) const
  {
  //this is invalid
  // std::cout << "calling invalid compute" << std::endl;
  // using InValueType = typename FieldInVecType::ComponentType;
  // using InDimensionTag = typename TypeTraits<InValueType>::DimensionalityTag;

  // using OutValueType = typename VecTraits<typename FieldOutType::ComponentType>::ComponentType;
  // using OutDimensionTag = typename TypeTraits<OutValueType>::DimensionalityTag;

  // std::cout << typeid(InDimensionTag).name() << '\n';
  // std::cout << typeid(FieldInVecType).name() << '\n';

  // std::cout << typeid(OutDimensionTag).name() << '\n';
  // std::cout << typeid(FieldOutType).name() << '\n';

  // std::cout << std::endl;
  }
};

}
} // namespace vtkm::worklet

#endif