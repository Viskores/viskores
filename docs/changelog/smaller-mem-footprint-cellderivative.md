#  CellDerivativeFor3DCell has a better version for Vec of Vec fields.
    
Previously we would compute a 3x3 matrix where each element was a Vec. Using
the jacobain of a single component is sufficient instead of computing it for
each component. This approach saves anywhere from 2 to 3 times the memory space.
