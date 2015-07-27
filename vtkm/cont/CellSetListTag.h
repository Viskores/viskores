//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSetListTag_h
#define vtk_m_cont_CellSetListTag_h

#ifndef VTKM_DEFAULT_CELL_SET_LIST_TAG
#define VTKM_DEFAULT_CELL_SET_LIST_TAG ::vtkm::cont::CellSetListTagCommon
#endif

#include <vtkm/ListTag.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>

namespace vtkm {
namespace cont {

struct CellSetListTagStructured1D
        : vtkm::ListTagBase<vtkm::cont::CellSetStructured<1> > {  };

struct CellSetListTagStructured2D
        : vtkm::ListTagBase<vtkm::cont::CellSetStructured<2> > {  };

struct CellSetListTagStructured3D
        : vtkm::ListTagBase<vtkm::cont::CellSetStructured<3> > {  };

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename IndexStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
struct CellSetListTagExplicit
        : vtkm::ListTagBase<
            vtkm::cont::CellSetExplicit<
              ShapeStorageTag,
              IndexStorageTag,
              ConnectivityStorageTag> > {  };

struct CellSetListTagExplicitDefault
        : CellSetListTagExplicit<> {  };

struct CellSetListTagCommon
        : vtkm::ListTagBase<
            vtkm::cont::CellSetStructured<2>,
            vtkm::cont::CellSetStructured<3>,
            vtkm::cont::CellSetExplicit<> > {  };

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetListTag_h
