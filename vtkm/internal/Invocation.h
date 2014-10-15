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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_internal_Invocation_h
#define vtk_m_internal_Invocation_h

#include <vtkm/Types.h>

namespace vtkm {
namespace internal {

/// \brief Container for types when dispatching worklets.
///
/// When a dispatcher and associated class invoke a worklet, they need to keep
/// track of the types of all parameters and the associated features of the
/// worklet. \c Invocation is a class that manages all these types.
///
template<typename _ParameterInterface,
         vtkm::IdComponent _InputDomainIndex>
struct Invocation
{
  /// \brief The types of the parameters
  ///
  /// \c ParameterInterface is (expected to be) a FunctionInterface class that
  /// lists the types of the parameters for the invocation.
  ///
  typedef _ParameterInterface ParameterInterface;

  /// \brief The index of the input domain.
  ///
  /// When a worklet is invoked, the pool of working threads is based of some
  /// constituent element of the input (such as the points or cells). This
  /// index points to the parameter that defines this input domain.
  ///
  static const vtkm::IdComponent InputDomainIndex = _InputDomainIndex;

  VTKM_CONT_EXPORT
  Invocation(ParameterInterface parameters) : Parameters(parameters) {  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c Parameters replaced.
  ///
  template<typename NewParameterInterface>
  struct ChangeParametersType {
    typedef Invocation<NewParameterInterface,InputDomainIndex> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c Parameters type is changed to the type given.
  ///
  template<typename NewParameterInterface>
  VTKM_EXEC_CONT_EXPORT
  typename ChangeParametersType<NewParameterInterface>::type
  ChangeParameters(NewParameterInterface newParameters) const {
    return typename ChangeParametersType<NewParameterInterface>::type(
          newParameters);
  }

  /// Defines a new \c Invocation type that is the same as this type except
  /// with the \c InputDomainIndex replaced.
  ///
  template<vtkm::IdComponent NewInputDomainIndex>
  struct ChangeInputDomainIndexType {
    typedef Invocation<ParameterInterface,NewInputDomainIndex> type;
  };

  /// Returns a new \c Invocation that is the same as this one except that the
  /// \c InputDomainIndex is changed to the static number given.
  ///
  template<vtkm::IdComponent NewInputDomainIndex>
  VTKM_EXEC_CONT_EXPORT
  typename ChangeInputDomainIndexType<NewInputDomainIndex>::type
  ChangeInputDomainIndex() const {
    return typename ChangeInputDomainIndexType<NewInputDomainIndex>::type(
          this->Parameters);
  }

  /// The state of an \c Invocation object holds the parameters of the
  /// invocation.
  ///
  ParameterInterface Parameters;
};

/// Convenience function for creating an Invocation object.
///
template<vtkm::IdComponent InputDomainIndex,
         typename ParameterInterface>
VTKM_CONT_EXPORT
vtkm::internal::Invocation<ParameterInterface, InputDomainIndex>
make_Invocation(const ParameterInterface &params)
{
  return vtkm::internal::Invocation<ParameterInterface,
                                    InputDomainIndex>(params);
}

}
} // namespace vtkm::internal

#endif //vtk_m_internal_Invocation_h
