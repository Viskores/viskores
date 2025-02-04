//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_examples_multibackend_IOWorker_h
#define viskores_examples_multibackend_IOWorker_h

#include "TaskQueue.h"
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

viskores::cont::DataSet make_test3DImageData(int xdim, int ydim, int zdim);
void io_generator(TaskQueue<viskores::cont::PartitionedDataSet>& queue, std::size_t numberOfTasks);

#endif
