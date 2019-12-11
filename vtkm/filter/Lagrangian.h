//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Lagrangian_h
#define vtk_m_filter_Lagrangian_h

#include <vtkm/filter/FilterDataSetWithField.h>

namespace vtkm
{
namespace filter
{

class Lagrangian : public vtkm::filter::FilterDataSetWithField<Lagrangian>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  Lagrangian();

  VTKM_CONT
  void SetRank(vtkm::Id val) { this->rank = val; }

  VTKM_CONT
  void SetInitFlag(bool val) { this->initFlag = val; }

  VTKM_CONT
  void SetExtractFlows(bool val) { this->extractFlows = val; }

  VTKM_CONT
  void SetResetParticles(bool val) { this->resetParticles = val; }

  VTKM_CONT
  void SetStepSize(vtkm::Float32 val) { this->stepSize = val; }

  VTKM_CONT
  void SetWriteFrequency(vtkm::Id val) { this->writeFrequency = val; }

  VTKM_CONT
  void SetSeedResolutionInX(vtkm::Id val) { this->x_res = val; }

  VTKM_CONT
  void SetSeedResolutionInY(vtkm::Id val) { this->y_res = val; }

  VTKM_CONT
  void SetSeedResolutionInZ(vtkm::Id val) { this->z_res = val; }

  VTKM_CONT
  void SetCustomSeedResolution(vtkm::Id val) { this->cust_res = val; }

  VTKM_CONT
  void SetSeedingResolution(vtkm::Id3 val) { this->SeedRes = val; }

  VTKM_CONT
  void UpdateSeedResolution(vtkm::cont::DataSet input);

  VTKM_CONT
  void WriteDataSet(vtkm::Id cycle, const std::string& filename, vtkm::cont::DataSet dataset);

  VTKM_CONT
  void InitializeUniformSeeds(const vtkm::cont::DataSet& input);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    vtkm::filter::PolicyBase<DerivedPolicy> policy);


  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::Id rank;
  bool initFlag;
  bool extractFlows;
  bool resetParticles;
  vtkm::Float32 stepSize;
  vtkm::Id x_res, y_res, z_res;
  vtkm::Id cust_res;
  vtkm::Id3 SeedRes;
  vtkm::Id writeFrequency;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Lagrangian.hxx>

#endif // vtk_m_filter_Lagrangian_h
