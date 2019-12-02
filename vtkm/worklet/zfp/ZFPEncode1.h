//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_zfp_encode1_h
#define vtk_m_worklet_zfp_encode1_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/zfp/ZFPBlockWriter.h>
#include <vtkm/worklet/zfp/ZFPEncode.h>
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
VTKM_EXEC inline void GatherPartial1(Scalar* q,
                                     const PortalType& scalars,
                                     vtkm::Id offset,
                                     int nx,
                                     int sx)
{
  vtkm::Id x;
  for (x = 0; x < nx; x++, offset += sx)
    q[x] = scalars.Get(offset);
  PadBlock(q, vtkm::UInt32(nx), 1);
}

template <typename Scalar, typename PortalType>
VTKM_EXEC inline void Gather1(Scalar* fblock, const PortalType& scalars, vtkm::Id offset, int sx)
{
  vtkm::Id counter = 0;

  for (vtkm::Id x = 0; x < 4; x++, offset += sx)
  {
    fblock[counter] = scalars.Get(offset);
    counter++;
  }
}

struct Encode1 : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id Dims;        // field dims
  vtkm::Id PaddedDims;  // dims padded to a multiple of zfp block size
  vtkm::Id ZFPDims;     // zfp block dims
  vtkm::UInt32 MaxBits; // bits per zfp block

public:
  Encode1(const vtkm::Id dims, const vtkm::Id paddedDims, const vtkm::UInt32 maxbits)
    : Dims(dims)
    , PaddedDims(paddedDims)
    , MaxBits(maxbits)
  {
    ZFPDims = PaddedDims / 4;
  }
  using ControlSignature = void(FieldIn, WholeArrayIn, AtomicArrayInOut bitstream);

  template <class InputScalarPortal, typename BitstreamPortal>
  VTKM_EXEC void operator()(const vtkm::Id blockIdx,
                            const InputScalarPortal& scalars,
                            BitstreamPortal& stream) const
  {
    using Scalar = typename InputScalarPortal::ValueType;

    vtkm::Id zfpBlock;
    zfpBlock = blockIdx % ZFPDims;
    vtkm::Id logicalStart = zfpBlock * vtkm::Id(4);

    constexpr vtkm::Int32 BlockSize = 4;
    Scalar fblock[BlockSize];

    bool partial = false;
    if (logicalStart + 4 > Dims)
      partial = true;

    if (partial)
    {
      const vtkm::Int32 nx =
        logicalStart + 4 > Dims ? vtkm::Int32(Dims - logicalStart) : vtkm::Int32(4);
      GatherPartial1(fblock, scalars, logicalStart, nx, 1);
    }
    else
    {
      Gather1(fblock, scalars, logicalStart, 1);
    }

    zfp::ZFPBlockEncoder<BlockSize, Scalar, BitstreamPortal> encoder;
    encoder.encode(fblock, static_cast<vtkm::Int32>(MaxBits), vtkm::UInt32(blockIdx), stream);
  }
};
}
}
}
#endif
