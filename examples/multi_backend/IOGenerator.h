//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_examples_multibackend_IOWorker_h
#define viskores_examples_multibackend_IOWorker_h

#include "TaskQueue.h"
#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>

viskores::cont::DataSet make_test3DImageData(int xdim, int ydim, int zdim);
void io_generator(TaskQueue<viskores::cont::PartitionedDataSet>& queue, std::size_t numberOfTasks);

#endif
