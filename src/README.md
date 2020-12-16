# Manual latest version installation

## Compile with CMake

Go to project root directory and run following commands 

```{shell}
mkdir build
cd build
cmake ..
make
make install
cd ..
```

Now you can use library from R

```{R}
library(devtools)
devtools::load_all()
...
```
