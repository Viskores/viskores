//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_decode1_h
#define vtk_m_worklet_zfp_decode1_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPDecode.h>
#include <vtkm/worklet/zfp/ZFPFunctions.h>
#include <vtkm/worklet/zfp/ZFPStructs.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void ScatterPartial1(const Scalar* q,
                                      PortalType& scalars,
                                      vtkm::Id offset,
                                      vtkm::Int32 nx)
{
  vtkm::Id x;
  for (x = 0; x < nx; x++, offset++, q++)
  {
    scalars.Set(offset, *q);
  }
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Scatter1(const Scalar* q, PortalType& scalars, vtkm::Id offset)
{
  for (vtkm::Id x = 0; x < 4; x++, ++offset)
  {
    scalars.Set(offset, *q++);
  } // x
}

struct Decode1 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id Dims;        // field dims
  vtkm::Id PaddedDims;  // dims padded to a multiple of zfp block size
  vtkm::Id ZFPDims;     // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block
public:
  Decode1(const vtkm::Id dims, const vtkm::Id paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayOut, WholeArrayIn bitstream);

  template <typename InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;
    constexpr vtkm::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];
    // clear
    for (vtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      fblock[i] = static_cast<Scalar>(0);
    }


    zfp::zfp_decode<BlockSize>(
      fblock, vtkm::Int32(MaxBits), static_cast<vtkm::UInt32>(blockIdx), stream);


    vtkm::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    vtkm::Id logicalStart = zfpBlock * vtkm::Id(4);

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;
    if (partial)
    {
      const vtkm::Int32 nx =
        logicalStart + 4 > Dims ? vtkm::Int32(Dims - logicalStart) : vtkm::Int32(4);
      ScatterPartial1(fblock, scalars, logicalStart, nx);
    }
    else
    {
      Scatter1(fblock, scalars, logicalStart);
    }
  }
};
}
}
} // namespace vtkm::worklet::zfp
#endif
