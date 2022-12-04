# Setting source parameters is more clear

Originally, most of the sources used constructor parameters to set the
various options of the source. Although convenient, it was difficult to
keep track of what each parameter meant. To make the code more clear,
source parameters are now set with accessor functions (e.g.
`SetPointDimensions`). Although this makes code more verbose, it helps
prevent mistakes and makes the changes more resilient to future changes.
