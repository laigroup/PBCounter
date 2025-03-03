# PBCounter

PBCounter is a Weighted Model Counter on Pseudo-Boolean Constraints. If you use the tool, please cite our paper [PBCounter: Weighted Model Counting on Pseudo-Boolean Formulas](https://link.springer.com/article/10.1007/s11704-024-3631-1)

## How to Use

You will need at least a C++ compiler that supports the C++17 standard. Default compiler is g++. To make changes, edit the CMakeLists.txt file.

Use `./build.sh` to compile executable file PBCounter.

Use `./PBCounter -h` or `PBCounter --help` to see help information.

## Additional Resources

You can access the benchmark dataset we used in our paper at [Zenodo](https://zenodo.org/records/14958540). 

Additionally, we provide the encoding tool [PBEncoder](https://github.com/laigroup/PBEncoder), which facilitates the compilation of PBFormula into CNF. The tool implements two counting-safe encoding methods used in our paper.
