#pragma once

#include "fault_manager.h"
#include "shared.h"
#include <cstdint>
#include <string>

#define NUM_FIELDS 6

// Default settings
#define DEF_TEA_IN_FILENAME "tea.in"
#define DEF_TEA_OUT_FILENAME "target/tea.out"
#define DEF_TEST_PROBLEM_FILENAME "tea.problems"
#define DEF_TEA_VISIT_FILENAME "tea.visit"
#define DEF_TEA_VTK_PATHNAME "target/vtk/"
#define DEF_FT false
#define DEF_WITH_FT_KILL_X 0
#define DEF_WITH_FT_KILL_Y 0
#define DEF_WITH_FT_KILL_ITER 0
#define DEF_FT_RECV_STRATEGY RecvFaultToleranceStrategy::INTERPOLATION
#define DEF_FT_STATIC_RECV_VALUE 0.00001
#define DEF_FT_RECV_INTERPOLATION_FACTOR 0.001
#define DEF_GRID_X_MIN 0.0
#define DEF_GRID_Y_MIN 0.0
#define DEF_GRID_Z_MIN 0.0
#define DEF_GRID_X_MAX 100.0
#define DEF_GRID_Y_MAX 100.0
#define DEF_GRID_Z_MAX 100.0
#define DEF_GRID_X_CELLS 10
#define DEF_GRID_Y_CELLS 10
#define DEF_GRID_Z_CELLS 10
#define DEF_DT_INIT 0.1
#define DEF_MAX_ITERS 10000
#define DEF_EPS 1.0E-15
#define DEF_END_TIME 10.0
#define DEF_END_STEP INT32_MAX
#define DEF_SUMMARY_FREQUENCY 10
#define DEF_VISIT_FREQUENCY 0
#define DEF_KERNEL_LANGUAGE C
#define DEF_COEFFICIENT CONDUCTIVITY
#define DEF_ERROR_SWITCH 0
#define DEF_PRESTEPS 30
#define DEF_EPS_LIM 1E-5
#define DEF_CHECK_RESULT 0
#define DEF_PPCG_INNER_STEPS 10
#define DEF_PRECONDITIONER 0
#define DEF_SOLVER Solver::CG_SOLVER
#define DEF_STAGING_BUFFER StagingBuffer::AUTO
#define DEF_NUM_STATES 0
#define DEF_NUM_CHUNKS 1
#define DEF_NUM_CHUNKS_PER_RANK 1
#define DEF_NUM_RANKS 1
#define DEF_HALO_DEPTH 2
#define DEF_RANK 0
#define DEF_IS_OFFLOAD false

// The type of solver to be run
enum class Solver { JACOBI_SOLVER, CG_SOLVER, CHEBY_SOLVER, PPCG_SOLVER };

// The language of the kernels to be run
enum class Kernel_Language { C, FORTRAN };

enum class StagingBuffer { ENABLE, DISABLE, AUTO };

enum class ModelKind { Host, Offload, Unified };

// The main settings structure
struct Settings {
  // Set of system-wide profiles
  Profile *kernel_profile;
  Profile *application_profile;
  Profile *wallclock_profile;

  // Log files
  FILE *tea_out_fp;

  int rank;
  int cart_rank;
  int *cart_coords;

  // Solve-wide constants
  int end_step;
  int presteps;
  int max_iters;
  int coefficient;
  int ppcg_inner_steps;
  int summary_frequency;
  int halo_depth;
  int num_states;
  int num_chunks;
  int num_chunks_per_rank;
  int num_ranks;
  bool *fields_to_exchange;

  bool is_offload;

  bool error_switch;
  bool check_result;
  bool preconditioner;

  double eps;
  double dt_init;
  double end_time;
  double eps_lim;

  // Input-Output files
  char *tea_in_filename;
  char *tea_out_filename;
  char *test_problem_filename;

  int visit_frequency;
  char *tea_visit_filename;
  char *tea_vtk_path_name;

  // Fault-tolerance config
  bool ft;
  int with_ft_kill_x;
  int with_ft_kill_y;
  int with_ft_kill_iter;
  RecvFaultToleranceStrategy ft_recv_strategy;
  double ft_recv_static_value;
  double ft_recv_interpolation_factor;

  Solver solver;
  char *solver_name;

  Kernel_Language kernel_language;
  char *device_selector;
  std::string model_name;
  ModelKind model_kind;
  StagingBuffer staging_buffer_preference;
  bool staging_buffer;

  // Field dimensions
  int grid_x_cells;
  int grid_y_cells;
  int grid_x_chunks;
  int grid_y_chunks;

  double grid_x_min;
  double grid_y_min;
  double grid_x_max;
  double grid_y_max;

  double dx;
  double dy;
};

// The accepted types of state geometry
enum class Geometry { RECTANGULAR, CIRCULAR, POINT };

// State list
struct State {
  bool defined;
  double density;
  double energy;
  double x_min;
  double y_min;
  double x_max;
  double y_max;
  double radius;
  Geometry geometry;
};

void set_default_settings(Settings &settings);
void reset_fields_to_exchange(Settings &settings);
bool is_fields_to_exchange(Settings &settings);
