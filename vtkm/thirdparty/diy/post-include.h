#undef VTKM_DIY_INCLUDE
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#if defined(VTKM_CLANG) || defined(VTKM_GCC)
#pragma GCC visibility pop
#endif

// When using an external DIY
// We need to alias the diy namespace to
// vtkmdiy so that VTK-m uses it properly
#if VTKM_USE_EXTERNAL_DIY
namespace vtkmdiy = ::diy;

#else
// The aliasing approach fails for when we
// want to use an internal version since
// the diy namespace already points to the
// external version. Instead we use macro
// replacement to make sure all diy classes
// are placed in vtkmdiy placed
#undef diy // mangle namespace diy

#endif
