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
#ifndef vtk_m_cont_arg_TransportTagTopologyIn_h
#define vtk_m_cont_arg_TransportTagTopologyIn_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/Types.h>

#include <vtkm/cont/CellSet.h>

#include <vtkm/cont/arg/Transport.h>

namespace vtkm {
namespace cont {
namespace arg {

/// \brief \c Transport tag for input arrays.
///
/// \c TransportTagTopologyIn is a tag used with the \c Transport class to
/// transport topology objects for input data.
///
struct TransportTagTopologyIn {  };

template<typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagTopologyIn, ContObjectType, Device>
{
  VTKM_IS_CELL_SET(ContObjectType);

  typedef typename ContObjectType
      ::template ConnectivityType<
          vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell>::Type
      ::template ExecutionTypes<Device>
      ::ExecObjectType ExecObjectType;

  VTKM_CONT_EXPORT
  ExecObjectType operator()(const ContObjectType &object, vtkm::Id) const
  {
    //create CUDA version of connectivity array.
    return object.GetNodeToCellConnectivity().PrepareForInput(Device());
  }
};

}
}
} // namespace vtkm::cont::arg

#endif //vtk_m_cont_arg_TransportTagTopologyIn_h
