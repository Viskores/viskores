//
// Created by ollie on 7/8/20.
//
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

/// Simulation of ferromagnetism using the Ising Model
/// Reference: Computational Physics 2nd Edition, Nicholas Giordano & Hisao Nakanishi

#include <iomanip>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View2D.h>
#include <vtkm/worklet/WorkletCellNeighborhood.h>

struct UpDown
{
  VTKM_EXEC_CONT vtkm::Float32 operator()(vtkm::Float32 p) const { return p > 0.5 ? 1.0f : -1.0f; }
};

vtkm::cont::DataSet SpinField(vtkm::Id2 dims)
{
  auto result =
    vtkm::cont::DataSetBuilderUniform::Create(dims, vtkm::Vec2f{ 0, 0 }, vtkm::Vec2f{ 1, 1 });

  vtkm::cont::ArrayHandle<vtkm::Float32> spins;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleTransform(
      vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(result.GetNumberOfCells()), UpDown{}),
    spins);
  result.AddCellField("spins", spins);

  return result;
}

struct UpdateSpins : public vtkm::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood prevspin,
                                FieldIn prob,
                                FieldOut spin);
  using ExecutionSignature = void(_2, _3, _4);

  template <typename NeighIn>
  VTKM_EXEC_CONT void operator()(const NeighIn& prevspin,
                                 vtkm::Float32 p,
                                 vtkm::Float32& spin) const
  {
    // TODO: what is the real value and unit of the change constant J and Boltzmann constant kB?
    const vtkm::Float32 J = 1.f;
    const vtkm::Float32 kB = 1.f;
    // TODO: temperature in Kelvin
    const vtkm::Float32 T = 5.f;
    const auto mySpin = prevspin.Get(0, 0, 0);

    // 1. Calculate the energy of flipping, E_flip
    vtkm::Float32 E_flip = J * mySpin *
      (prevspin.Get(-1, -1, 0) + prevspin.Get(-1, 0, 0) + prevspin.Get(-1, 1, 0) +
       prevspin.Get(0, -1, 0) + prevspin.Get(0, 1, 0) + prevspin.Get(1, -1, 0) +
       prevspin.Get(1, 0, 0) + prevspin.Get(1, 1, 0));

    if (E_flip <= 0)
    {
      // 2. If E_flip <= 0, just flip the spin
      spin = -1.f * mySpin;
    }
    else
    {
      // 3. otherwise, flip the spin if the Boltzmann factor exp(-E_flip/kB*T) is larger than the
      // uniform real random number p.
      if (p <= vtkm::Exp(-E_flip / (kB * T)))
        spin = -1.f * mySpin;
      else
        spin = mySpin;
    }
  }
};

int main(int argc, char** argv)
{
  auto opts =
    vtkm::cont::InitializeOptions::DefaultAnyDevice | vtkm::cont::InitializeOptions::Strict;
  vtkm::cont::Initialize(argc, argv, opts);

  auto dataSet = SpinField({ 5, 5 });
  vtkm::cont::ArrayHandle<vtkm::Float32> spins;
  dataSet.GetCellField("spins").GetData().AsArrayHandle(spins);

  vtkm::rendering::Scene scene;
  vtkm::rendering::Actor actor(dataSet.GetCellSet(),
                               dataSet.GetCoordinateSystem(),
                               dataSet.GetCellField("spins"),
                               vtkm::cont::ColorTable("Cool To Warm"));
  scene.AddActor(actor);
  vtkm::rendering::CanvasRayTracer canvas(1024, 1024);
  vtkm::rendering::MapperRayTracer mapper;
  mapper.SetShadingOn(false);
  vtkm::rendering::View2D view(scene, mapper, canvas);
  view.Paint();
  view.SaveAs("spin0.png");

  vtkm::cont::Invoker invoker;
  for (vtkm::UInt32 i = 1; i < 10; ++i)
  {
    vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32> prob(dataSet.GetNumberOfCells(), { i });
    invoker(UpdateSpins{}, dataSet.GetCellSet(), spins, prob, spins);
    view.Paint();
    view.SaveAs("spin" + std::to_string(i) + ".png");
  }
}
