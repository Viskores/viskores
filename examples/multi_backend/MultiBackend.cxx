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
#include <iostream>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

struct GenerateSurfaceWorklet : public vtkm::worklet::WorkletMapField
{
  vtkm::Float32 t;
  GenerateSurfaceWorklet(vtkm::Float32 st) : t(st) {}

  typedef void ControlSignature( FieldIn<>, FieldOut<>, FieldOut<> );
  typedef void ExecutionSignature( _1, _2, _3 );

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()( const vtkm::Vec< T, 3 > & input,
                   vtkm::Vec<T, 3> & output,
                   vtkm::Vec<vtkm::UInt8, 4>& color ) const
  {
    output[0] = input[0];
    output[1] = 0.25f * vtkm::Sin( input[0] * 10.f + t ) * vtkm::Cos( input[2] * 10.f + t );
    output[2] = input[2];

    color[0] = 0;
    color[1] = static_cast<vtkm::UInt8>(160 + (96 * vtkm::Sin(input[0] * 10.f + t)));
    color[2] = static_cast<vtkm::UInt8>(160 + (96 * vtkm::Cos(input[2] * 5.f + t)));
    color[3] = 255;
  }
};

template<bool> struct CanRun;

template<typename T, typename DeviceAdapterTag>
void run_if_valid(vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > inHandle,
                  vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > outCoords,
                  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::UInt8, 4 > > outColors,
                  DeviceAdapterTag tag)
{
 typedef vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>
         DeviceAdapterTraits;

  if(DeviceAdapterTraits::Valid)
    {
    std::cout << "Running a worklet on device adapter: "
              << DeviceAdapterTraits::GetId() << std::endl;
    }
  else
    {
    std::cout << "Unable to run a worklet on device adapter: "
              << DeviceAdapterTraits::GetId() << std::endl;
    }

  CanRun<DeviceAdapterTraits::Valid>::run(inHandle,outCoords,outColors,tag);
}


//Implementation that we call on device adapters we don't have support
//enabled for
template<>
struct CanRun<false>
{
  template<typename T, typename DeviceAdapterTag>
  static void run(vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > vtkmNotUsed(inHandle),
                  vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > vtkmNotUsed(outCoords),
                  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::UInt8, 4 > > vtkmNotUsed(outColors),
                  DeviceAdapterTag)
  {
  }
};

//Implementation that we call on device adapters we do have support
//enabled for
template<>
struct CanRun<true>
{
  template<typename T, typename DeviceAdapterTag>
  static void run(vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > inHandle,
                  vtkm::cont::ArrayHandle< vtkm::Vec< T, 3 > > outCoords,
                  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::UInt8, 4 > > outColors,
                  DeviceAdapterTag)
  {

  //even though we have support for this device adapter we haven't determined
  //if we actually have run-time support. This is a significant issue with
  //the CUDA backend
  vtkm::cont::RuntimeDeviceInformation<DeviceAdapterTag> runtime;
  const bool haveSupport = runtime.Exists();

  if(haveSupport)
    {
    typedef vtkm::worklet::DispatcherMapField<GenerateSurfaceWorklet,
                                              DeviceAdapterTag> DispatcherType;

    GenerateSurfaceWorklet worklet( 0.05f );
    DispatcherType(worklet).Invoke( inHandle,
                                    outCoords,
                                    outColors);
    }
  }
};

template<typename T>
std::vector< vtkm::Vec<T, 3> > make_testData(int size)
{
  std::vector< vtkm::Vec< T, 3 > > data;
  data.reserve( static_cast<std::size_t>(size*size) );
  for (int i = 0; i < size; ++i )
    {
    for (int j = 0; j < size; ++j )
      {
      data.push_back( vtkm::Vec<T,3>( 2.f * i / size - 1.f,
                                      0.f,
                                      2.f * j / size - 1.f ) );
      }
    }
  return data;
}


int main(int, char**)
{
  typedef vtkm::Vec< vtkm::Float32, 3 > FloatVec3;
  typedef vtkm::Vec< vtkm::UInt8, 4 > Uint8Vec4;

  std::vector< FloatVec3 > data = make_testData<vtkm::Float32>(1024);

  typedef ::vtkm::cont::DeviceAdapterTagSerial SerialTag;
  typedef ::vtkm::cont::DeviceAdapterTagTBB TBBTag;
  typedef ::vtkm::cont::DeviceAdapterTagCuda CudaTag;

  //make array handles for the data
  vtkm::cont::ArrayHandle< FloatVec3 > in = vtkm::cont::make_ArrayHandle(data);
  vtkm::cont::ArrayHandle< FloatVec3 > out;
  vtkm::cont::ArrayHandle< Uint8Vec4 > color;

  //Run the algorithm on all backends that we have compiled support for.
  run_if_valid(in, out, color, CudaTag());
  run_if_valid(in, out, color, TBBTag());
  run_if_valid(in, out, color, SerialTag());
}


