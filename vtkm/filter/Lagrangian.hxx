//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string.h>

static vtkm::Id cycle = 0;
static vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> BasisParticles;
static vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> BasisParticlesOriginal;
static vtkm::cont::ArrayHandle<vtkm::Id> BasisParticlesValidity;

class ValidityCheck : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldIn steps, FieldInOut output);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  inline VTKM_CONT void SetBounds(vtkm::Bounds b) { bounds = b; }

  template <typename PosType, typename StepType, typename ValidityType>
  VTKM_EXEC void operator()(const PosType& end_point,
                            const StepType& steps,
                            ValidityType& res) const
  {
    if (steps > 0 && res == 1)
    {
      if (end_point[0] >= bounds.X.Min && end_point[0] <= bounds.X.Max &&
          end_point[1] >= bounds.Y.Min && end_point[1] <= bounds.Y.Max &&
          end_point[2] >= bounds.Z.Min && end_point[2] <= bounds.Z.Max)
      {
        res = 1;
      }
      else
      {
        res = 0;
      }
    }
    else
    {
      res = 0;
    }
  }

private:
  vtkm::Bounds bounds;
};

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Lagrangian::Lagrangian()
  : vtkm::filter::FilterDataSetWithField<Lagrangian>()
  , rank(0)
  , initFlag(true)
  , extractFlows(false)
  , resetParticles(true)
  , stepSize(1.0f)
  , x_res(0)
  , y_res(0)
  , z_res(0)
  , cust_res(0)
  , SeedRes(vtkm::Id3(1, 1, 1))
  , writeFrequency(0)
{
}

//-----------------------------------------------------------------------------
inline void Lagrangian::WriteDataSet(vtkm::Id cycle,
                                     std::string filename,
                                     vtkm::cont::DataSet dataset)
{
  std::stringstream file;
  file << filename << cycle << ".vtk";
  vtkm::io::writer::VTKDataSetWriter writer(file.str().c_str());
  writer.WriteDataSet(dataset);
  std::cout << "Number of flows in writedataset is : " << dataset.GetCellSet(0).GetNumberOfCells()
            << std::endl;
}

//-----------------------------------------------------------------------------
inline void Lagrangian::UpdateSeedResolution(const vtkm::cont::DataSet input)
{
  vtkm::cont::DynamicCellSet cell_set = input.GetCellSet();

  if (cell_set.IsSameType(vtkm::cont::CellSetStructured<1>()))
  {
    vtkm::cont::CellSetStructured<1> cell_set1 = cell_set.Cast<vtkm::cont::CellSetStructured<1>>();
    vtkm::Id dims1 = cell_set1.GetPointDimensions();
    this->SeedRes[0] = dims1;
    if (this->cust_res)
    {
      this->SeedRes[0] = dims1 / this->x_res;
    }
  }
  else if (cell_set.IsSameType(vtkm::cont::CellSetStructured<2>()))
  {
    vtkm::cont::CellSetStructured<2> cell_set2 = cell_set.Cast<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 dims2 = cell_set2.GetPointDimensions();
    this->SeedRes[0] = dims2[0];
    this->SeedRes[1] = dims2[1];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims2[0] / this->x_res;
      this->SeedRes[1] = dims2[1] / this->y_res;
    }
  }
  else if (cell_set.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    vtkm::cont::CellSetStructured<3> cell_set3 = cell_set.Cast<vtkm::cont::CellSetStructured<3>>();
    vtkm::Id3 dims3 = cell_set3.GetPointDimensions();
    this->SeedRes[0] = dims3[0];
    this->SeedRes[1] = dims3[1];
    this->SeedRes[2] = dims3[2];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims3[0] / this->x_res;
      this->SeedRes[1] = dims3[1] / this->y_res;
      this->SeedRes[2] = dims3[2] / this->z_res;
    }
  }
}


//-----------------------------------------------------------------------------
inline void Lagrangian::InitializeUniformSeeds(const vtkm::cont::DataSet& input)
{
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  Lagrangian::UpdateSeedResolution(input);

  vtkm::Float64 x_spacing = 0.0, y_spacing = 0.0, z_spacing = 0.0;
  if (this->SeedRes[0] > 1)
    x_spacing = (double)(bounds.X.Max - bounds.X.Min) / (double)(this->SeedRes[0] - 1);
  if (this->SeedRes[1] > 1)
    y_spacing = (double)(bounds.Y.Max - bounds.Y.Min) / (double)(this->SeedRes[1] - 1);
  if (this->SeedRes[2] > 1)
    z_spacing = (double)(bounds.Z.Max - bounds.Z.Min) / (double)(this->SeedRes[2] - 1);
  // Divide by zero handling for 2D data set. How is this handled

  BasisParticles.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
  BasisParticlesValidity.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);

  auto portal1 = BasisParticles.GetPortalControl();
  auto portal2 = BasisParticlesValidity.GetPortalControl();

  vtkm::Id count = 0;
  for (int x = 0; x < this->SeedRes[0]; x++)
  {
    for (int y = 0; y < this->SeedRes[1]; y++)
    {
      for (int z = 0; z < this->SeedRes[2]; z++)
      {
        portal1.Set(count,
                    vtkm::Vec<vtkm::Float64, 3>(bounds.X.Min + (x * x_spacing),
                                                bounds.Y.Min + (y * y_spacing),
                                                bounds.Z.Min + (z * z_spacing)));
        portal2.Set(count, 1);
        count++;
      }
    }
  }
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Lagrangian::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{

  if (cycle == 0)
  {
    InitializeUniformSeeds(input);
    BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
    vtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
  }

  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->writeFrequency == 0)
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Write frequency can not be 0. Use SetWriteFrequency().");
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> basisParticleArray;
  vtkm::cont::ArrayCopy(BasisParticles, basisParticleArray);

  cycle += 1;
  std::cout << "Cycle : " << cycle << std::endl;
  const vtkm::cont::DynamicCellSet& cells =
    input.GetCellSet(this->GetActiveCoordinateSystemIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  vtkm::worklet::ParticleAdvection particleadvection;
  vtkm::worklet::ParticleAdvectionResult res;

  GridEvalType gridEval(coords, cells, field);
  RK4Type rk4(gridEval, static_cast<vtkm::Float32>(this->stepSize));
  res = particleadvection.Run(rk4, basisParticleArray, 1); // Taking a single step

  auto particle_positions = res.positions;
  auto particle_stepstaken = res.stepsTaken;

  auto start_position = BasisParticlesOriginal.GetPortalControl();
  auto end_position = particle_positions.GetPortalControl();

  auto portal_stepstaken = particle_stepstaken.GetPortalControl();
  auto portal_validity = BasisParticlesValidity.GetPortalControl();

  vtkm::cont::DataSet outputData;
  vtkm::cont::DataSetBuilderExplicit dataSetBuilder;

  if (cycle % this->writeFrequency == 0)
  {
    int connectivity_index = 0;
    std::vector<vtkm::Id> connectivity;
    std::vector<vtkm::Vec<T, 3>> pointCoordinates;
    std::vector<vtkm::UInt8> shapes;
    std::vector<vtkm::IdComponent> numIndices;

    for (vtkm::Id index = 0; index < res.positions.GetNumberOfValues(); index++)
    {
      auto start_point = start_position.Get(index);
      auto end_point = end_position.Get(index);
      auto steps = portal_stepstaken.Get(index);

      if (steps > 0 && portal_validity.Get(index) == 1)
      {
        if (end_point[0] >= bounds.X.Min && end_point[0] <= bounds.X.Max &&
            end_point[1] >= bounds.Y.Min && end_point[1] <= bounds.Y.Max &&
            end_point[2] >= bounds.Z.Min && end_point[2] <= bounds.Z.Max)
        {
          connectivity.push_back(connectivity_index);
          connectivity.push_back(connectivity_index + 1);
          connectivity_index += 2;
          pointCoordinates.push_back(
            vtkm::Vec<T, 3>((float)start_point[0], (float)start_point[1], (float)start_point[2]));
          pointCoordinates.push_back(
            vtkm::Vec<T, 3>((float)end_point[0], (float)end_point[1], (float)end_point[2]));
          shapes.push_back(vtkm::CELL_SHAPE_LINE);
          numIndices.push_back(2);
        }
        else
        {
          portal_validity.Set(index, 0);
        }
      }
      else
      {
        portal_validity.Set(index, 0);
      }
    }

    outputData = dataSetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
    std::stringstream file_path;
    file_path << "output/basisflows_" << this->rank << "_";
    std::cout << "Writing basis flows to output/" << std::endl;
    WriteDataSet(cycle, file_path.str().c_str(), outputData);
    if (this->resetParticles)
    {
      InitializeUniformSeeds(input);
      BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
      vtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
    }
    else
    {
      vtkm::cont::ArrayCopy(particle_positions, BasisParticles);
    }
  }
  else
  {
    ValidityCheck check;
    check.SetBounds(bounds);
    vtkm::worklet::DispatcherMapField<ValidityCheck> dispatcher(check);
    dispatcher.Invoke(particle_positions, particle_stepstaken, BasisParticlesValidity);
    vtkm::cont::ArrayCopy(particle_positions, BasisParticles);
  }

  return outputData;
}

//---------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Lagrangian::DoMapField(vtkm::cont::DataSet&,
                                             const vtkm::cont::ArrayHandle<T, StorageType>&,
                                             const vtkm::filter::FieldMetadata&,
                                             const vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter
