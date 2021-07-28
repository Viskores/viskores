#include <vtkm/thirdparty/diy/Configure.h>

#if VTKM_USE_EXTERNAL_DIY
#define VTKM_DIY_INCLUDE(header) <diy/header>
#else
#define VTKM_DIY_INCLUDE(header) <vtkmdiy/header>
#define diy vtkmdiy // mangle namespace diy
#endif

#if defined(VTKM_CLANG) || defined(VTKM_GCC)
#pragma GCC visibility push(default)
#endif

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
