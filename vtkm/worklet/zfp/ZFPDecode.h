#ifndef vtk_m_worklet_zfp_decode_h
#define vtk_m_worklet_zfp_decode_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>
#include <vtkm/worklet/zfp/ZFPBlockReader.h>
#include <vtkm/worklet/zfp/ZFPCodec.h>
#include <vtkm/worklet/zfp/ZFPTypeInfo.h>

namespace vtkm
{
namespace worklet
{
namespace zfp
{


template <typename Int, typename Scalar>
VTKM_EXEC Scalar dequantize(const Int& x, const int& e);

template <>
VTKM_EXEC vtkm::Float64 dequantize<vtkm::Int64, vtkm::Float64>(const vtkm::Int64& x,
                                                               const vtkm::Int32& e)
{
  return vtkm::Ldexp((vtkm::Float64)x, e - (CHAR_BIT * scalar_sizeof<vtkm::Float64>() - 2));
}

template <>
VTKM_EXEC vtkm::Float32 dequantize<vtkm::Int32, vtkm::Float32>(const vtkm::Int32& x,
                                                               const vtkm::Int32& e)
{
  return vtkm::Ldexp((vtkm::Float32)x, e - (CHAR_BIT * scalar_sizeof<vtkm::Float32>() - 2));
}

template <>
VTKM_EXEC vtkm::Int32 dequantize<vtkm::Int32, vtkm::Int32>(const vtkm::Int32&, const vtkm::Int32&)
{
  return 1;
}

template <>
VTKM_EXEC vtkm::Int64 dequantize<vtkm::Int64, vtkm::Int64>(const vtkm::Int64&, const vtkm::Int32&)
{
  return 1;
}

template <class Int, vtkm::UInt32 s>
VTKM_EXEC static void inv_lift(Int* p)
{
  Int x, y, z, w;
  x = *p;
  p += s;
  y = *p;
  p += s;
  z = *p;
  p += s;
  w = *p;
  p += s;

  /*
  ** non-orthogonal transform
  **       ( 4  6 -4 -1) (x)
  ** 1/4 * ( 4  2  4  5) (y)
  **       ( 4 -2  4 -5) (z)
  **       ( 4 -6 -4  1) (w)
  */
  y += w >> 1;
  w -= y >> 1;
  y += w;
  w <<= 1;
  w -= y;
  z += x;
  x <<= 1;
  x -= z;
  y += z;
  z <<= 1;
  z -= y;
  w += x;
  x <<= 1;
  x -= w;

  p -= s;
  *p = w;
  p -= s;
  *p = z;
  p -= s;
  *p = y;
  p -= s;
  *p = x;
}

template <vtkm::Int64 BlockSize>
struct inv_transform;

template <>
struct inv_transform<64>
{
  template <typename Int>
  VTKM_EXEC void inv_xform(Int* p)
  {
    uint x, y, z;
    /* transform along z */
    for (y = 0; y < 4; y++)
      for (x = 0; x < 4; x++)
        inv_lift<Int, 16>(p + 1 * x + 4 * y);
    /* transform along y */
    for (x = 0; x < 4; x++)
      for (z = 0; z < 4; z++)
        inv_lift<Int, 4>(p + 16 * z + 1 * x);
    /* transform along x */
    for (z = 0; z < 4; z++)
      for (y = 0; y < 4; y++)
        inv_lift<Int, 1>(p + 4 * y + 16 * z);
  }
};

template <>
struct inv_transform<16>
{
  template <typename Int>
  VTKM_EXEC void inv_xform(Int* p)
  {

    for (int x = 0; x < 4; ++x)
    {
      inv_lift<Int, 4>(p + 1 * x);
    }
    for (int y = 0; y < 4; ++y)
    {
      inv_lift<Int, 1>(p + 4 * y);
    }
  }
};

template <>
struct inv_transform<4>
{
  template <typename Int>
  VTKM_EXEC void inv_xform(Int* p)
  {
    inv_lift<Int, 1>(p);
  }
};


VTKM_EXEC
vtkm::Int64 uint2int(vtkm::UInt64 x)
{
  return (x ^ 0xaaaaaaaaaaaaaaaaull) - 0xaaaaaaaaaaaaaaaaull;
}

VTKM_EXEC
vtkm::Int32 uint2int(vtkm::UInt32 x)
{
  return (x ^ 0xaaaaaaaau) - 0xaaaaaaaau;
}

// Note: I die a little inside everytime I write this sort of template
template <vtkm::Int32 BlockSize,
          typename PortalType,
          template <int Size, typename Portal> class ReaderType,
          typename UInt>
VTKM_EXEC void decode_ints(ReaderType<BlockSize, PortalType>& reader,
                           vtkm::Int32& maxbits,
                           UInt* data,
                           const vtkm::Int32 intprec)
{
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    data[i] = 0;
  }

  vtkm::UInt64 x;
  const vtkm::UInt32 kmin = 0;
  vtkm::Int32 bits = maxbits;
  for (vtkm::UInt32 k = intprec, n = 0; bits && k-- > kmin;)
  {
    // read bit plane
    vtkm::UInt32 m = vtkm::Min(n, vtkm::UInt32(bits));
    bits -= m;
    x = reader.read_bits(m);
    for (; n < BlockSize && bits && (bits--, reader.read_bit()); x += (Word)1 << n++)
      for (; n < (BlockSize - 1) && bits && (bits--, !reader.read_bit()); n++)
        ;

    // deposit bit plane
    for (int i = 0; x; i++, x >>= 1)
    {
      data[i] += (UInt)(x & 1u) << k;
    }
  }
  //for (int i = 0; i < BlockSize; i++)
  //{
  //  std::cout<<"Decomp int "<<i<<" = "<<data[i]<<"\n";
  //}
}

template <vtkm::Int32 BlockSize, typename Scalar, typename Int>
void zfp_convert(Scalar inv_w, Scalar* fblock, Int* iblock)
{
  for (vtkm::Int32 i = 0; i < BlockSize; ++i)
  {
    fblock[i] = inv_w * (Scalar)iblock[i];
  }
}
template <vtkm::Int32 BlockSize, typename Scalar, typename PortalType>
VTKM_EXEC void zfp_decode(Scalar* fblock,
                          vtkm::Int32 maxbits,
                          vtkm::UInt32 blockIdx,
                          PortalType stream)
{
  zfp::BlockReader<BlockSize, PortalType> reader(stream, maxbits, blockIdx);
  using Int = typename zfp::zfp_traits<Scalar>::Int;
  using UInt = typename zfp::zfp_traits<Scalar>::UInt;

  vtkm::UInt32 cont = 1;

  if (!zfp::is_int<Scalar>())
  {
    cont = reader.read_bit();
  }

  if (cont)
  {
    vtkm::UInt32 ebits = zfp::get_ebits<Scalar>() + 1;

    vtkm::UInt32 emax;
    if (!zfp::is_int<Scalar>())
    {
      //std::cout<<"ebits "<<ebits<<"\n";
      // read in the shared exponent
      //vtkm::UInt64 b = reader.read_bits(ebits - 1);
      //print_bits(b);
      //std::cout<<"b "<<b<<"\n";
      //std::cout<<"ebias "<<zfp::get_ebias<Scalar>()<<"\n";
      //emax = vtkm::UInt32(b - zfp::get_ebias<Scalar>());
      emax = vtkm::UInt32(reader.read_bits(ebits - 1) - zfp::get_ebias<Scalar>());
      //std::cout<<"EMAX "<<emax<<"\n";
    }
    else
    {
      // no exponent bits
      ebits = 0;
    }

    maxbits -= ebits;
    UInt ublock[BlockSize];
    decode_ints<BlockSize>(reader, maxbits, ublock, zfp::get_precision<Scalar>());

    Int iblock[BlockSize];
    const zfp::ZFPCodec<BlockSize> codec;
    for (vtkm::Int32 i = 0; i < BlockSize; ++i)
    {
      vtkm::UInt8 idx = codec.CodecLookup(i);
      iblock[idx] = uint2int(ublock[i]);
    }

    //for (int i = 0; i < BlockSize; i++)
    //{
    //  std::cout<<"before xform tid "<<i<<"  "<<iblock[i]<<"\n";
    //}

    inv_transform<BlockSize> trans;
    trans.inv_xform(iblock);

    //for (int i = 0; i < BlockSize; i++)
    //{
    //  std::cout<<"tid "<<i<<"  "<<iblock[i]<<"\n";
    //}

    Scalar inv_w = dequantize<Int, Scalar>(1, emax);

    //std::cout<<"dequantize factor "<<inv_w<<"\n";

    zfp_convert<BlockSize>(inv_w, fblock, iblock);
  }
}
}
}
} // namespace vtkm::worklet::zfp
#endif
