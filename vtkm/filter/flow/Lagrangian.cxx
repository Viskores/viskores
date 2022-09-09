//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/ErrorFilterExecution.h>

#include <vtkm/filter/flow/Lagrangian.h>
#include <vtkm/filter/flow/worklet/Field.h>
#include <vtkm/filter/flow/worklet/GridEvaluators.h>
#include <vtkm/filter/flow/worklet/ParticleAdvection.h>
#include <vtkm/filter/flow/worklet/RK4Integrator.h>
#include <vtkm/filter/flow/worklet/Stepper.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

namespace
{
class ValidityCheck : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  ValidityCheck(vtkm::Bounds b)
    : bounds(b)
  {
  }

  template <typename ValidityType>
  VTKM_EXEC void operator()(const vtkm::Particle& end_point, ValidityType& res) const
  {
    vtkm::Id steps = end_point.NumSteps;
    if (steps > 0 && res == 1)
    {
      if (bounds.Contains(end_point.Pos))
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

class DisplacementCalculation : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldIn start_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename DisplacementType>
  VTKM_EXEC void operator()(const vtkm::Particle& end_point,
                            const vtkm::Particle& start_point,
                            DisplacementType& res) const
  {
    res[0] = end_point.Pos[0] - start_point.Pos[0];
    res[1] = end_point.Pos[1] - start_point.Pos[1];
    res[2] = end_point.Pos[2] - start_point.Pos[2];
  }
};
}

//-----------------------------------------------------------------------------
void Lagrangian::UpdateSeedResolution(const vtkm::cont::DataSet input)
{
  vtkm::cont::UnknownCellSet cell_set = input.GetCellSet();

  if (cell_set.CanConvert<vtkm::cont::CellSetStructured<1>>())
  {
    vtkm::cont::CellSetStructured<1> cell_set1 =
      cell_set.AsCellSet<vtkm::cont::CellSetStructured<1>>();
    vtkm::Id dims1 = cell_set1.GetPointDimensions();
    this->SeedRes[0] = dims1;
    if (this->CustRes)
    {
      this->SeedRes[0] = dims1 / this->ResX;
    }
  }
  else if (cell_set.CanConvert<vtkm::cont::CellSetStructured<2>>())
  {
    vtkm::cont::CellSetStructured<2> cell_set2 =
      cell_set.AsCellSet<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 dims2 = cell_set2.GetPointDimensions();
    this->SeedRes[0] = dims2[0];
    this->SeedRes[1] = dims2[1];
    if (this->CustRes)
    {
      this->SeedRes[0] = dims2[0] / this->ResX;
      this->SeedRes[1] = dims2[1] / this->ResY;
    }
  }
  else if (cell_set.CanConvert<vtkm::cont::CellSetStructured<3>>())
  {
    vtkm::cont::CellSetStructured<3> cell_set3 =
      cell_set.AsCellSet<vtkm::cont::CellSetStructured<3>>();
    vtkm::Id3 dims3 = cell_set3.GetPointDimensions();
    this->SeedRes[0] = dims3[0];
    this->SeedRes[1] = dims3[1];
    this->SeedRes[2] = dims3[2];
    if (this->CustRes)
    {
      this->SeedRes[0] = dims3[0] / this->ResX;
      this->SeedRes[1] = dims3[1] / this->ResY;
      this->SeedRes[2] = dims3[2] / this->ResZ;
    }
  }
}


//-----------------------------------------------------------------------------
void Lagrangian::InitializeSeedPositions(const vtkm::cont::DataSet& input)
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

  this->BasisParticles.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
  this->BasisParticlesValidity.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);

  auto portal1 = this->BasisParticles.WritePortal();
  auto portal2 = this->BasisParticlesValidity.WritePortal();

  vtkm::Id id = 0;
  for (int z = 0; z < this->SeedRes[2]; z++)
  {
    vtkm::FloatDefault zi = static_cast<vtkm::FloatDefault>(z * z_spacing);
    for (int y = 0; y < this->SeedRes[1]; y++)
    {
      vtkm::FloatDefault yi = static_cast<vtkm::FloatDefault>(y * y_spacing);
      for (int x = 0; x < this->SeedRes[0]; x++)
      {
        vtkm::FloatDefault xi = static_cast<vtkm::FloatDefault>(x * x_spacing);
        portal1.Set(id,
                    vtkm::Particle(Vec3f(static_cast<vtkm::FloatDefault>(bounds.X.Min) + xi,
                                         static_cast<vtkm::FloatDefault>(bounds.Y.Min) + yi,
                                         static_cast<vtkm::FloatDefault>(bounds.Z.Min) + zi),
                                   id));
        portal2.Set(id, 1);
        id++;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void Lagrangian::InitializeCoordinates(const vtkm::cont::DataSet& input,
                                       std::vector<Float64>& xC,
                                       std::vector<Float64>& yC,
                                       std::vector<Float64>& zC)
{
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  vtkm::Float64 x_spacing = 0.0, y_spacing = 0.0, z_spacing = 0.0;
  if (this->SeedRes[0] > 1)
    x_spacing = (double)(bounds.X.Max - bounds.X.Min) / (double)(this->SeedRes[0] - 1);
  if (this->SeedRes[1] > 1)
    y_spacing = (double)(bounds.Y.Max - bounds.Y.Min) / (double)(this->SeedRes[1] - 1);
  if (this->SeedRes[2] > 1)
    z_spacing = (double)(bounds.Z.Max - bounds.Z.Min) / (double)(this->SeedRes[2] - 1);
  // Divide by zero handling for 2D data set. How is this handled

  for (int x = 0; x < this->SeedRes[0]; x++)
  {
    vtkm::FloatDefault xi = static_cast<vtkm::FloatDefault>(x * x_spacing);
    xC.push_back(bounds.X.Min + xi);
  }
  for (int y = 0; y < this->SeedRes[1]; y++)
  {
    vtkm::FloatDefault yi = static_cast<vtkm::FloatDefault>(y * y_spacing);
    yC.push_back(bounds.Y.Min + yi);
  }
  for (int z = 0; z < this->SeedRes[2]; z++)
  {
    vtkm::FloatDefault zi = static_cast<vtkm::FloatDefault>(z * z_spacing);
    zC.push_back(bounds.Z.Min + zi);
  }
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Lagrangian::DoExecute(const vtkm::cont::DataSet& input)
{
  if (this->Cycle == 0)
  {
    this->InitializeSeedPositions(input);
    vtkm::cont::ArrayCopy(this->BasisParticles, this->BasisParticlesOriginal);
  }

  if (this->WriteFrequency == 0)
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Write frequency can not be 0. Use SetWriteFrequency().");
  }
  vtkm::cont::ArrayHandle<vtkm::Particle> basisParticleArray;
  vtkm::cont::ArrayCopy(this->BasisParticles, basisParticleArray);

  this->Cycle += 1;
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::flow::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::flow::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::flow::Stepper<RK4Type, GridEvalType>;

  vtkm::worklet::flow::ParticleAdvection particleadvection;
  vtkm::worklet::flow::ParticleAdvectionResult<vtkm::Particle> res;

  const auto field = input.GetField(this->GetActiveFieldName());
  FieldType velocities(field.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Vec3f>>(),
                       field.GetAssociation());

  GridEvalType gridEval(coords, cells, velocities);
  Stepper rk4(gridEval, static_cast<vtkm::Float32>(this->StepSize));

  res = particleadvection.Run(rk4, basisParticleArray, 1); // Taking a single step
  auto particles = res.Particles;

  vtkm::cont::DataSet outputData;
  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;

  if (this->Cycle % this->WriteFrequency == 0)
  {
    /* Steps to create a structured dataset */
    UpdateSeedResolution(input);
    vtkm::cont::ArrayHandle<vtkm::Vec3f> basisParticlesDisplacement;
    basisParticlesDisplacement.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
    DisplacementCalculation displacement;
    this->Invoke(displacement, particles, this->BasisParticlesOriginal, basisParticlesDisplacement);
    std::vector<Float64> xC, yC, zC;
    InitializeCoordinates(input, xC, yC, zC);
    outputData = dataSetBuilder.Create(xC, yC, zC);
    outputData.AddPointField("valid", this->BasisParticlesValidity);
    outputData.AddPointField("displacement", basisParticlesDisplacement);

    if (this->ResetParticles)
    {
      this->InitializeSeedPositions(input);
      vtkm::cont::ArrayCopy(this->BasisParticles, this->BasisParticlesOriginal);
    }
    else
    {
      vtkm::cont::ArrayCopy(particles, this->BasisParticles);
    }
  }
  else
  {
    ValidityCheck check(bounds);
    this->Invoke(check, particles, this->BasisParticlesValidity);
    vtkm::cont::ArrayCopy(particles, this->BasisParticles);
  }

  return outputData;
}

}
}
} //vtkm::filter::flow


//Deprecated filter: vtkm::filter::Lagrangian
namespace vtkm
{
namespace filter
{

static vtkm::Id deprecatedLagrange_Cycle = 0;
static vtkm::cont::ArrayHandle<vtkm::Particle> deprecatedLagrange_BasisParticles;
static vtkm::cont::ArrayHandle<vtkm::Particle> deprecatedLagrange_BasisParticlesOriginal;
static vtkm::cont::ArrayHandle<vtkm::Id> deprecatedLagrange_BasisParticlesValidity;


VTKM_CONT vtkm::cont::DataSet Lagrangian::DoExecute(const vtkm::cont::DataSet& input)
{
  //Initialize the filter with the static variables
  this->SetCycle(deprecatedLagrange_Cycle);
  this->SetBasisParticles(deprecatedLagrange_BasisParticles);
  this->SetBasisParticlesOriginal(deprecatedLagrange_BasisParticlesOriginal);
  this->SetBasisParticleValidity(deprecatedLagrange_BasisParticlesValidity);

  //call the base class
  auto output = vtkm::filter::flow::Lagrangian::DoExecute(input);

  //Set the static variables with the current values.
  deprecatedLagrange_Cycle = this->GetCycle();
  deprecatedLagrange_BasisParticles = this->GetBasisParticles();
  deprecatedLagrange_BasisParticlesOriginal = this->GetBasisParticlesOriginal();
  deprecatedLagrange_BasisParticlesValidity = this->GetBasisParticleValidity();

  return output;
}

}
} // namespace vtkm::filter
