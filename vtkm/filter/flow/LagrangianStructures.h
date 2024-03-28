//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_LagrangianStructures_h
#define vtk_m_filter_flow_LagrangianStructures_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// @brief Compute the finite time Lyapunov exponent (FTLE) of a vector field.
///
/// The FTLE is computed by advecting particles throughout the vector field and analyizing
/// where they diverge or converge. By default, the points of the input `vtkm::cont::DataSet`
/// are all advected for this computation unless an auxiliary grid is established.
///
class VTKM_FILTER_FLOW_EXPORT LagrangianStructures : public vtkm::filter::Filter
{
public:
  VTKM_CONT
  bool CanThread() const override { return false; }

  /// @brief Specifies the step size used for the numerical integrator.
  ///
  /// The numerical integrators operate by advancing each particle by a finite amount.
  /// This parameter defines the distance to advance each time. Smaller values are
  /// more accurate but take longer to integrate. An appropriate step size is usually
  /// around the size of each cell.
  void SetStepSize(vtkm::FloatDefault s) { this->StepSize = s; }
  /// @copydoc SetStepSize
  vtkm::FloatDefault GetStepSize() { return this->StepSize; }

  /// @brief Specify the maximum number of steps each particle is allowed to traverse.
  ///
  /// This can limit the total length of displacements used when computing the FTLE.
  void SetNumberOfSteps(vtkm::Id n) { this->NumberOfSteps = n; }
  /// @copydoc SetNumberOfSteps
  vtkm::Id GetNumberOfSteps() { return this->NumberOfSteps; }

  /// @brief Specify the time interval for the advection.
  ///
  /// The FTLE works by advecting all points a finite distance, and this parameter
  /// specifies how far to advect.
  void SetAdvectionTime(vtkm::FloatDefault advectionTime) { this->AdvectionTime = advectionTime; }
  /// @copydoc SetAdvectionTime
  vtkm::FloatDefault GetAdvectionTime() { return this->AdvectionTime; }

  /// @brief Specify whether to use an auxiliary grid.
  ///
  /// When this flag is off (the default), then the points of the mesh representing the vector
  /// field are advected and used for computing the FTLE. However, if the mesh is too coarse,
  /// the FTLE will likely be inaccurate. Or if the mesh is unstructured the FTLE may be less
  /// efficient to compute. When this flag is on, an auxiliary grid of uniformly spaced points
  /// is used for the FTLE computation.
  void SetUseAuxiliaryGrid(bool useAuxiliaryGrid) { this->UseAuxiliaryGrid = useAuxiliaryGrid; }
  /// @copydoc SetUseAuxiliaryGrid
  bool GetUseAuxiliaryGrid() { return this->UseAuxiliaryGrid; }

  /// @brief Specify the dimensions of the auxiliary grid for FTLE calculation.
  ///
  /// Seeds for advection will be placed along the points of this auxiliary grid.
  /// This option has no effect unless the UseAuxiliaryGrid option is on.
  void SetAuxiliaryGridDimensions(vtkm::Id3 auxiliaryDims) { this->AuxiliaryDims = auxiliaryDims; }
  /// @copydoc SetAuxiliaryGridDimensions
  vtkm::Id3 GetAuxiliaryGridDimensions() { return this->AuxiliaryDims; }

  /// @brief Specify whether to use flow maps instead of advection.
  ///
  /// If the start and end points for FTLE calculation are known already, advection is
  /// an unnecessary step. This flag allows users to bypass advection, and instead use
  /// a precalculated flow map. By default this option is off.
  void SetUseFlowMapOutput(bool useFlowMapOutput) { this->UseFlowMapOutput = useFlowMapOutput; }
  /// @copydoc SetUseFlowMapOutput
  bool GetUseFlowMapOutput() { return this->UseFlowMapOutput; }

  /// @brief Specify the name of the output field in the data set returned.
  ///
  /// By default, the field will be named `FTLE`.
  void SetOutputFieldName(std::string outputFieldName) { this->OutputFieldName = outputFieldName; }
  /// @copydoc SetOutputFieldName
  std::string GetOutputFieldName() { return this->OutputFieldName; }

  /// @brief Specify the array representing the flow map output to be used for FTLE calculation.
  inline void SetFlowMapOutput(vtkm::cont::ArrayHandle<vtkm::Vec3f>& flowMap)
  {
    this->FlowMapOutput = flowMap;
  }
  /// @copydoc SetFlowMapOutput
  inline vtkm::cont::ArrayHandle<vtkm::Vec3f> GetFlowMapOutput() { return this->FlowMapOutput; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

  vtkm::FloatDefault AdvectionTime;
  vtkm::Id3 AuxiliaryDims;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> FlowMapOutput;
  std::string OutputFieldName = "FTLE";
  vtkm::FloatDefault StepSize = 1.0f;
  vtkm::Id NumberOfSteps = 0;
  bool UseAuxiliaryGrid = false;
  bool UseFlowMapOutput = false;
};

}
}
} // namespace vtkm

#endif // vtk_m_filter_flow_LagrangianStructures_h
