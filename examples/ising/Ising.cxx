//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/Initialize.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View2D.h>
#include <viskores/worklet/WorkletCellNeighborhood.h>

struct UpDown
{
  VISKORES_EXEC_CONT viskores::Float32 operator()(viskores::Float32 p) const
  {
    return p > 0.5 ? 1.0f : -1.0f;
  }
};

viskores::cont::DataSet SpinField(viskores::Id2 dims)
{
  auto result = viskores::cont::DataSetBuilderUniform::Create(
    dims, viskores::Vec2f{ 0, 0 }, viskores::Vec2f{ 1, 1 });

  viskores::cont::ArrayHandle<viskores::Float32> spins;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandleTransform(
      viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32>(result.GetNumberOfCells()),
      UpDown{}),
    spins);
  result.AddCellField("spins", spins);

  return result;
}

struct UpdateSpins : public viskores::worklet::WorkletCellNeighborhood
{
  using ControlSignature = void(CellSetIn,
                                FieldInNeighborhood prevspin,
                                FieldIn prob,
                                FieldOut spin);
  using ExecutionSignature = void(_2, _3, _4);

  template <typename NeighIn>
  VISKORES_EXEC_CONT void operator()(const NeighIn& prevspin,
                                     viskores::Float32 p,
                                     viskores::Float32& spin) const
  {
    // TODO: what is the real value and unit of the change constant J and Boltzmann constant kB?
    const viskores::Float32 J = 1.f;
    const viskores::Float32 kB = 1.f;
    // TODO: temperature in Kelvin
    const viskores::Float32 T = 5.f;
    const auto mySpin = prevspin.Get(0, 0, 0);

    // 1. Calculate the energy of flipping, E_flip
    viskores::Float32 E_flip = J * mySpin *
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
      if (p <= viskores::Exp(-E_flip / (kB * T)))
        spin = -1.f * mySpin;
      else
        spin = mySpin;
    }
  }
};

int main(int argc, char** argv)
{
  auto opts =
    viskores::cont::InitializeOptions::DefaultAnyDevice | viskores::cont::InitializeOptions::Strict;
  viskores::cont::Initialize(argc, argv, opts);

  auto dataSet = SpinField({ 5, 5 });
  viskores::cont::ArrayHandle<viskores::Float32> spins;
  dataSet.GetCellField("spins").GetData().AsArrayHandle(spins);

  viskores::rendering::Scene scene;
  viskores::rendering::Actor actor(dataSet, "spins", viskores::cont::ColorTable("Cool To Warm"));
  scene.AddActor(actor);
  viskores::rendering::CanvasRayTracer canvas(1024, 1024);
  viskores::rendering::MapperRayTracer mapper;
  mapper.SetShadingOn(false);
  viskores::rendering::View2D view(scene, mapper, canvas);
  view.Paint();
  view.SaveAs("spin0.png");

  viskores::cont::Invoker invoker;
  for (viskores::UInt32 i = 1; i < 10; ++i)
  {
    viskores::cont::ArrayHandleRandomUniformReal<viskores::Float32> prob(dataSet.GetNumberOfCells(),
                                                                         { i });
    invoker(UpdateSpins{}, dataSet.GetCellSet(), spins, prob, spins);
    view.Paint();
    view.SaveAs("spin" + std::to_string(i) + ".png");
  }
}
