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
#ifndef vtk_m_cont_arg_Transport_h
#define vtk_m_cont_arg_Transport_h

namespace vtkm {
namespace cont {
namespace arg {

/// \brief Class for transporting from the control to the execution environment.
///
/// The \c Transport class is used to transport data of a certain type from the
/// control environment to the execution environment. It is used internally in
/// VTK-m's dispatch mechanism.
///
/// \c Transport is a templated class with three arguments. The first argument
/// is a tag declaring the mechanism of transport. The second argument is the
/// type of data to transport. The third argument is device adapter tag for
/// the device to move the data to.
///
/// There is no generic implementation of \c Transport. There are partial
/// specializations of \c Transport for each mechanism supported. If you get a
/// compiler error about an incomplete type for \c Transport, it means you used
/// an invalid \c TransportTag or it is an invalid combination of data type or
/// device adapter.
///
template<typename TransportTag,
         typename ContObjectType,
         typename DeviceAdapterTag>
struct Transport
#ifdef VTKM_DOXYGEN_ONLY
{
  /// \brief The type used in the execution environment.
  ///
  /// All \c Transport specializations are expected to declare a type named \c
  /// ExecObjectType that is the object type used in the execution environment.
  /// For example, for an \c ArrayHandle, the \c ExecObjectType is the portal
  /// used in the execution environment.
  ///
  typedef typename ContObjectType::
      template ExecutionTypes<DeviceAdapterTag>::PortalConst ExecObjectType;

  /// \brief Send data to the execution environment.
  ///
  /// All \c Transport specializations are expected to have a constant
  /// parenthesis operator that takes the data in the control environment and
  /// returns an object that is accessible in the execution environment. The
  /// operator also has a second argument that is the size of the dispatch that
  /// can be used, for example, to allocate data for an output array.
  ///
  VTKM_CONT_EXPORT
  ExecObjectType operator()(const ContObjectType contData, vtkm::Id size) const;
};
#else // VTKM_DOXYGEN_ONLY
    ;
#endif // VTKM_DOXYGEN_ONLY

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_Transport_h
