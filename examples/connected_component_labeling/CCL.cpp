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

#include <algorithm>
#include <iostream>
#include <random>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/cont/TryExecute.h>
//#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
//#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

void populate(std::vector<vtkm::Float32>& pixels,
              std::vector<vtkm::Id>& components,
              vtkm::UInt32 width,
              vtkm::UInt32 height)
{
  for (auto& pixel : pixels) {
    pixel = 0.0f;
  }

  for (int i = 6; i <= 8; i++) {
    pixels[i] = 1.0f;
  }

  for (int i = 16; i <= 18; i++) {
    pixels[i] = 1.0f;
  }

  for (int i = 0; i < width*height; ++i) {
    components[i] = i;
  }
}

struct UpdateComponent : public vtkm::worklet::WorkletPointNeighborhood3x3x3
{
  typedef void ControlSignature(FieldInNeighborhood<Scalar> pixel,
                                CellSetIn,
                                FieldInNeighborhood<IdType> prevComponent,
                                FieldOut<IdType> component);

  typedef void ExecutionSignature(_1, _3, _4);

  //verify input domain can be something other than first parameter
  typedef _2 InputDomain;

  template <typename FieldIn, typename FieldIn1, typename FieldOut>
  VTKM_EXEC void operator()(const vtkm::exec::arg::Neighborhood<1, FieldIn>& pixel,
                            const vtkm::exec::arg::Neighborhood<1, FieldIn1>& prevComponent,
                            FieldOut& component) const
                            //FieldIn1& component) const
  {
    //vtkm::UInt8 color =
    std::cout << "pixel: " << pixel.Get(0, 0, 0)
              << ", component: " << prevComponent.Get(0, 0, 0)
              << std::endl;

    vtkm::Float32 color = pixel.Get(0, 0, 0);
    if (color == pixel.Get(-1, 0, 0)) {
      component = prevComponent.Get(-1, 0, 0);
    } else if (color == pixel.Get(0, -1, 0)) {
      component = prevComponent.Get(0, -1, 0);
    } else {
      component = prevComponent.Get(0, 0, 0);
    }
  }
};

struct FindRoot : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<IdType>,
                                WholeArrayInOut<IdType> component);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 Inputdomain;

  template <typename FieldIn, typename FieldOut>
  VTKM_EXEC void operator()(const FieldIn& id, FieldOut& component) const {
    //while (id != component.Get(id)) {
//    if (id != component.Get(id)) {
//      std::cout << "id: " << id << ", component: " << component.Get(id) << std::endl;
//      component.Set(id, this->operator()(component.Get(id), component));
//    }
    vtkm::Id parent = component.Get(id);
    while (parent != component.Get(parent)) {
      parent = component.Get(parent);
      component.Set(id, parent);

      std::cout << "id: " << id << ", component: " << component.Get(id) << std::endl;
    }
  };
};

int main(int argc, char *argv[])
{
  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet data = builder.Create(vtkm::Id2(5, 5));
  std::vector<vtkm::Float32> pixels(5*5);
  std::vector<vtkm::Id> components(5*5);
  populate(pixels, components, 5, 5);
  vtkm::cont::Field pixelField("pixels", vtkm::cont::Field::ASSOC_POINTS, pixels);
  data.AddField(pixelField);

  vtkm::cont::Field componentField("components", vtkm::cont::Field::ASSOC_POINTS, components);
  data.AddField(componentField);

  vtkm::worklet::DispatcherPointNeighborhood<UpdateComponent> dispatcher;

  vtkm::cont::ArrayHandle<vtkm::Id> output;
  dispatcher.Invoke(data.GetField("pixels"),
                    data.GetCellSet(),
                    data.GetField("components"),
                    output);

  for (int i = 0; i < output.GetNumberOfValues(); i++) {
    std::cout << output.GetPortalConstControl().Get(i) << " ";
  }
  std::cout << std::endl;

  vtkm::worklet::DispatcherMapField<FindRoot> findRootDispatcher;
  findRootDispatcher.Invoke(data.GetField("components"), output);
   for (int i = 0; i < output.GetNumberOfValues(); i++) {
    std::cout << output.GetPortalConstControl().Get(i) << " ";
  }
  std::cout << std::endl;
}