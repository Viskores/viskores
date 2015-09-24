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
#ifndef vtk_m_exec_arg_CellShape_h
#define vtk_m_exec_arg_CellShape_h

#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

namespace vtkm {
namespace exec {
namespace arg {

/// \brief Aspect tag to use for getting the cell shape.
///
/// The \c AspectTagCellShape aspect tag causes the \c Fetch class to
/// obtain the type of element (e.g. cell cell) from the topology object.
///
struct AspectTagCellShape {  };

/// \brief The \c ExecutionSignature tag to use to get the cell shape.
///
struct CellShape : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static const vtkm::IdComponent INDEX = 1;
  typedef vtkm::exec::arg::AspectTagCellShape AspectTag;
};

template<typename FetchTag,
         typename Invocation,
         vtkm::IdComponent ParameterIndex>
struct Fetch<
    FetchTag, vtkm::exec::arg::AspectTagCellShape, Invocation, ParameterIndex>
{
  // The parameter for the input domain is stored in the Invocation. (It is
  // also in the worklet, but it is safer to get it from the Invocation
  // in case some other dispatch operation had to modify it.)
  static const vtkm::IdComponent InputDomainIndex =
      Invocation::InputDomainIndex;

  // ParameterInterface (from Invocation) is a FunctionInterface type
  // containing types for all objects passed to the Invoke method (with some
  // dynamic casting performed so objects like DynamicArrayHandle get cast to
  // ArrayHandle) and then transferred to the execution environment. This
  // interface contains a set of exec objects.
  typedef typename Invocation::ParameterInterface ParameterInterface;

  // This is the type for the input domain (derived from the last two things we
  // got from the Invocation). Assuming the input domain is set up correctly,
  // this should be one of the vtkm::exec::Connectivity* classes.
  typedef typename ParameterInterface::
      template ParameterType<InputDomainIndex>::type ConnectivityType;

  typedef typename ConnectivityType::CellShapeTag ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_EXPORT
  ValueType Load(vtkm::Id index, const Invocation &invocation) const
  {
    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    ConnectivityType connectivity =
        invocation.Parameters.template GetParameter<InputDomainIndex>();

    return connectivity.GetCellShape(index);
  }

  VTKM_EXEC_EXPORT
  void Store(vtkm::Id, const Invocation &, const ValueType &) const
  {
    // Store is a no-op.
  }
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_CellShape_h
