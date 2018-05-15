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
#ifndef vtk_m_examples_multibackend_IOWorker_h
#define vtk_m_examples_multibackend_IOWorker_h

#include "TaskQueue.h"
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/MultiBlock.h>

vtkm::cont::DataSet make_test3DImageData(int xdim, int ydim, int zdim);
void io_generator(TaskQueue<vtkm::cont::MultiBlock>& queue, std::size_t numberOfTasks);

#endif
