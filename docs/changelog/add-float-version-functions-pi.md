# Add float version operations for vtkm::Math Pi()

Now PI related functions are evalulated at compile time as constexpr functions.
It also removes the old static_cast<T>vtkm::Pi() usages with
template ones and fix several conversion warnings.
