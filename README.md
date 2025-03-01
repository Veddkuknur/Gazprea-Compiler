# Usage
## Installing MLIR
In this project you will be working with MLIR and LLVM.
Due to the complex nature (and size) of the project we did not want to include
MLIR as a subproject. Therefore, there is some additional setup required to get
your build up and running.

### On a personal machine
  1. Follow the instructions on the
     [setup page](https://cmput415.github.io/415-docs/setup/cs_computers.html)
     for your machine.
     
## Building
### Linux
  1. Install git, java (only the runtime is necessary), and cmake (>= v3.0).
     - Until now, cmake has found the dependencies without issues. If you
       encounter an issue, let a TA know and we can fix it.
  1. Make a directory that you intend to build the project in and change into
     that directory.
  1. Run `cmake <path-to-Gazprea-Base>`.
  1. Run `make`.
  1. Done.
