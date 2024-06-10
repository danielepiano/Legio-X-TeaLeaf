# Legio X TeaLeaf

Project for **_Advanced Computer Architectures_** course @ **Politecnico di Milano**.

[**_TeaLeaf_**](https://github.com/UoB-HPC/TeaLeaf) heat conduction <u>mini-app</u> over
[**_Legio_**](https://github.com/Robyroc/Legio) MPI fault-tolerance library.

The objective is to perform a performance and functionality analysis in scenarios where the MPI infrastructure is
affected by failures.

## 👨‍👨‍👦‍👦 Authors

- Daniele Piano [@danielepiano](https://github.com/danielepiano)
- Federico Martinis [@MartinisFederico](https://github.com/MartinisFederico)
- Roberto Rocco [@Robyroc](https://github.com/Robyroc) :: as supervisor
- prof. [Gianluca Palermo](https://palermo.faculty.polimi.it/) :: as project responsible

## _TeaLeaf_ :: About

> A C++based implementation of the TeaLeaf heat conduction mini-app. This implementation of TeaLeaf replicates the
> functionality of the reference version of TeaLeaf (https://github.com/UK-MAC/TeaLeaf_ref).
>
>This implementation has support for building with and without MPI. When MPI is enabled, all models will adjust
> accordingly for asynchronous MPI send/recv.

## _Legio_ :: About

> Legio is a library that introduces fault-tolerance in MPI applications in the form of graceful degradation.
> It's designed for embarrassingly parallel applications. It is based on ULFM.
>
> One of the key aspects of Legio is the transparency of integration: no changes in the code are needed, integration is
> performed via linking.
> Legio leverages PMPI to catch all the calls toward MPI and wraps them with the appropriate code needed.

## _TeaLeaf_ :: Programming models

Together with MPI, _TeaLeaf_ is currently implemented in the following parallel programming models, listed in no
particular order.

| NAME                      | COMPILER REFERENCE |
|---------------------------|:------------------:|
| -                         |      `serial`      |
| OpenMP 3, 4.5             |       `omp`        |
| CUDA                      |       `cuda`       |
| HIP                       |       `hip`        |
| Kokkos >= 4               |      `kokkos`      |
| C++ Parallel STL (StdPar) |   `std-indices`    |
| SYCL                      |     `sycl-acc`     |
| SYCL 2020                 |  ^ or `sycl-usm`   |

## Installing _Legio_

### Prerequisites

* CMake >= 3.10
* ULFM features in MPI implementation

### Steps

Follow the steps defined in [_Legio_](https://github.com/Robyroc/Legio) repository.

## Building _Legio-X-TeaLeaf_

### Prerequisites

* CMake >= 3.13

### Steps

```shell
# Configure the build
# 'Release' as default build-type
# -DMODEL option is required
foo@bar:~/path/to/Legio-X-TeaLeaf$ cmake -Bbuild -H. -DMODEL=<model> -DENABLE_MPI=ON <model options through -D...> -Dlegio_DIR=<path/to/Legio/install/lib/cmake>

# Compile
foo@bar:~/path/to/Legio-X-TeaLeaf$ cmake --build build
```

### Notes

* `MODEL` :: selects the **programming model** implementation of _TeaLeaf_ to build (**references** shown above); the
  source code for each model's implementations is located in `./src/<model>`

## Executing _Legio-X-TeaLeaf_

### Steps

```shell
# Run executables in ./build
foo@bar:~/path/to/Legio-X-TeaLeaf$ <mpirun> -n <num_ranks> --with-ft ulfm ./build/<model>-tealeaf
```

### Notes

* `<mpirun>` :: _mpirun_ executable with ULFM features
* `--with-ft ulfm` :: fault-tolerance support via ULFM (built-in by default in _OpenMPI v5.0.x_)
* `./build/<model>-tealeaf` :: executable path and filename generated according to the defined `model

## _TeaLeaf_ :: File Input

The contents of _tea.in_ defines the geometric and runtime information, apart from task and thread counts.

A complete list of options is given below, where `<R>` shows the option takes a real number as an argument.
Similarly `<I>` is an integer argument.

There is not a full implementation of the configuration properties of the _TeaLeaf_ref_ application.

| OPTION                                                                                    | DESCRIPTION                                                                                                                                                                                                                                                                                                                                         |
|-------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `xmin <R>`<br/>`xmax <R>`<br/>`ymin <R>`<br/>`ymax <R>`                                   | Size of the computational domain. The default domain size is a 10cm square.                                                                                                                                                                                                                                                                         |
| `x_cells <I>`<br/>`y_cells <I>`                                                           | Number of discrete cells through which decompose the computational domain along the two axis. The default is 10 cells in each direction.                                                                                                                                                                                                            |
| `state 1 density <R> energy <R>`                                                          | State of the ambient material of the computational domain - here geometry information is ignored. Regions not covered by other defined states receive the energy and density of state 1.                                                                                                                                                            |
| `state <I> density <R> energy <R> geometry rectangle xmin <R> ymin <R> xmax <R> ymax <R>` | State of a rectangular region in the computational domain.<br/>Note that the generator is simple and the defined state <u>completely</u> fills a cell with which it intersects.<br/>In case of over lapping regions, the last state takes priority.                                                                                                 |
| `state <I> density <R> energy <R> geometry circular xmin <R> ymin <R> radius <R>`         | State of a circular region in the computational domain.<br/>Note that the generator is simple and the defined state <u>completely</u> fills a cell with which it intersects.<br/>In case of over lapping regions, the last state takes priority.<br/>Hence, a circular region will have a stepped interface.                                        |
| `state <I> density <R> energy <R> geometry point xmin <R> ymin <R>`                       | State of a point in the computational domain.<br/>Note that the generator is simple and the defined state <u>completely</u> fills a cell with which it intersects.<br/>In case of over lapping regions, the last state takes priority.<br/>Hence, a point region will fill the cell it lies in.                                                     |
| `visit_frequency <I>`                                                                     | Step frequency of visualisation dumps. The files produced are text base VTK files and are easily viewed on apps such as _ViSit_. The default is to output no graphical data. The default is to output no graphical data.<br/>Note that the overhead of output is high, so should not be invoked when performance benchmarking is being carried out. |
| `summary_frequency <I>`                                                                   | Step frequency of summary dumps. This requires a global reduction and associated synchronisation, so performance will be slightly affected as the frequency is increased. The default is for a summary dump to be produced every 10 steps and at the end of the simulation.                                                                         |
| `initial_timestep <R>`                                                                    | Initial time step. This time step stays constant through the entire simulation. The default value is 0.1.                                                                                                                                                                                                                                           |
| `end_time <R>`                                                                            | End time for the simulation. When the simulation time is greater than this number the simulation will stop.                                                                                                                                                                                                                                         |
| `end_step <I>`                                                                            | Number of the end step for the simulation. When the simulation step is equal to this then simulation will stop. In case both this and the previous options are set, the simulation will terminate on whichever completes first.                                                                                                                     |
| `preconditioner_on`                                                                       | Whether to apply a preconditioner before linear solving.</br>N.d.R. This property seems read but not used throughout the _TeaLeaf_ code.                                                                                                                                                                                                            |
| `use_jacobi`                                                                              | _Jacobi_ method to solve the linear system. Note that this a very slowly converging method compared to other options. This is the default method is no method is explicitly selected.                                                                                                                                                               |
| `use_cg`                                                                                  | _Conjugate Gradient_ method to solve the linear system.                                                                                                                                                                                                                                                                                             |
| `use_ppcg`                                                                                | _Conjugate Gradient_ method to solve the linear system.                                                                                                                                                                                                                                                                                             |
| `use_chebyshev`                                                                           | _Chebyshev_ method to solve the linear system.                                                                                                                                                                                                                                                                                                      |
| `presteps <I>`                                                                            | Number of _Conjugate Gradient_ iterations to be completed before the _Chebyshev_ method is started. This is necessary to provide approximate minimum and maximum eigen values to start the _Chebyshev_ method. The default value is 30.                                                                                                             |
| `ppcg_inner_steps <I>`                                                                    | Number of inner steps to run when using the _PPCG_ solver. The default value is 10.                                                                                                                                                                                                                                                                 |
| `errswitch`                                                                               | If enabled alongside _Chebshev_/_PPCG_ solver, switch when a certain error is reached instead of when a certain number of steps is reached. The default for this is off.                                                                                                                                                                            |
| `epslim`                                                                                  | Default error to switch from _CG_ to _Chebyshev_ when using _Chebyshev_ solver with the `tl_cg_ch_errswitch` option enabled. The default value is 1e-5.                                                                                                                                                                                             |
| `max_iters <I>`                                                                           | Provides an upper limit of the number of iterations used for the linear solve in a step. If this limit is reached, then the solution vector at this iteration is used as the solution <u>anyway</u>. The default value is 1000.                                                                                                                     |
| `eps <R>`                                                                                 | Convergence criteria for the selected solver. It uses the least squares measure of the residual. The default value is 1.0e-10.                                                                                                                                                                                                                      |
| `coefficient_density`                                                                     | Use the density as the conduction coefficient. This is the default option.                                                                                                                                                                                                                                                                          |
| `coefficient_inverse_density`                                                             | Use the inverse density as the conduction coefficient.                                                                                                                                                                                                                                                                                              |
| `halo_depth`                                                                              |                                                                                                                                                                                                                                                                                                                                                     |
| `num_chunks_per_rank`                                                                     | N.d.R. Actually, settable but works only with 1 chunk per rank.                                                                                                                                                                                                                                                                                     |

New properties added by _TeaLeaf_ w.r.t. to _TeaLeaf_ref_ are

| OPTION                | DESCRIPTION                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `check_result`        | Standard test with a "known" solution.<br/>Solutions are iterated until the right sequence of `x_cells`, `y_cells` and `end_step` is found.<br/>Note that the known solution for an iterative solver is not an analytic solution but is the solution for a single core simulation with IEEE options enabled with the Intel compiler and a strict convergence of 1.0e-15.<br/>The difference with the expected solution is reported at the end of simulation in the _tea.out_ file.<br/>There is no default value for this option. |
| `use_fortran_kernels` |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `use_c_kernels`       |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

Following properties have been implemented in this fork.

| OPTION                | DESCRIPTION                                                                                                                                                                                                                                                                                                              |
|-----------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `visit_frequency <I>` | Step frequency of visualisation dumps. The files produced are text base VTK files and are easily viewed on apps such as _ViSit_, _ParaView_, etc.. The default is to output no graphical data.<br/>Note that the visit overhead is high, so it should not be invoked when performance benchmarking is being carried out. |

## _Legio-X-TeaLeaf_ postprocessing

Just like _TeaLeaf_ref_, this application has been improved to make each node produce its own VTK file - _Visualization
ToolKit_ format. Each VTK file can be opened and visualized in applications such as _ViSit_ and _ParaView_.

To improve VTK files management on these applications, a postprocessing script is supplied to merge VTK files produced
by different nodes but related to the same iteration.

### Prerequisites

* Python >= 3
* PIP >= 24.0

### Steps

```shell
# UNA TANTUM :: Install required packages
foo@bar:~/path/to/Legio-X-TeaLeaf$ pip3 install -r postprocess-requirements.txt

# Postprocess
foo@bar:~/path/to/Legio-X-TeaLeaf$ python3 postprocess.py [-options]
```

Following, the options to run the script.

| OPTION                                         | DEFAULT                | DESCRIPTION                                                                              |
|------------------------------------------------|------------------------|------------------------------------------------------------------------------------------|
| `-i <string>` </br> `--input <string>`         | target/vtk             | The directory containing the input VTK files.                                            |
| `-o <string>` </br> `--output <string>`        | target/vtk/postprocess | The directory to produce the merged VTK files in.                                        |
| `-p <string>` </br> `--output-prefix <string>` | tea                    | The prefix to introduce to output VTK filenames for `<prefix>.<x>.<y>.<iteratino> naming. |
| `-v <string>` </br> `--visit <string>`         | target/vtk             | The directory containing the 'tea.visit' file.    `                                       |
| `--bin <bool>`                                 | false                  | Whether the VTK files should be generated in a bin`ary format.                            |
| `--rm <bool>`                                  | false                  | Whether to remove the VTK files in the input directory after postprocessing.             |

## Utilities

### Shortcut to delete all VTK files in default paths

Useful if you want to run another execution with either different `end_step` or `visit_frequency`.

```shell
# Remove all target/**/*.vtk files
foo@bar:~/path/to/Legio-X-TeaLeaf$ ./clear-vtk.sh
```