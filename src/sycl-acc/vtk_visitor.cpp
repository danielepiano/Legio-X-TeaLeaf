#include "vtk_visitor.h"
#include "comms.h"
#include <iomanip>
#include <fstream>
#include <sstream>

#define MAX_CHAR_LEN 256

void track_all_vtk_files(int time_step, Settings &settings);
void visit_vtk_file(int time_step, Chunk *chunks, Settings &settings);

char *calc_x_y_time_step_filename(int x, int y, int time_step) {
  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  std::ostringstream dynamic_filename;
  dynamic_filename << "tea." << std::setfill('0') << std::setw(5) << x;
  dynamic_filename << "." << std::setfill('0') << std::setw(5) << y;
  dynamic_filename << "." << std::setfill('0') << std::setw(5) << time_step;
  dynamic_filename << ".vtk";
  strncpy(filename, dynamic_filename.str().c_str(), MAX_CHAR_LEN);
  return filename;
}

void init(Settings &settings) {
  if (settings.rank != MASTER) {
    return;
  }
  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  strncpy(filename, settings.tea_vtk_path_name, MAX_CHAR_LEN);
  strcat(filename, settings.tea_visit_filename);

  std::ofstream tea_visit(filename, std::ofstream::out);
  tea_visit << "grid_y_chunks " << settings.grid_y_chunks << std::endl;
  tea_visit << "grid_x_chunks " << settings.grid_x_chunks << std::endl;
  tea_visit.close();
}

void visit(int time_step, Chunk *chunks, Settings &settings) {
  if (!time_step) {
    init(settings);
  }

  barrier();
  track_all_vtk_files(time_step, settings);
  barrier();

  visit_vtk_file(time_step, chunks, settings);
  barrier();
}

void track_all_vtk_files(int time_step, Settings &settings) {
  if (settings.rank != MASTER) {
    return;
  }

  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  strncpy(filename, settings.tea_vtk_path_name, MAX_CHAR_LEN);
  strcat(filename, settings.tea_visit_filename);
  std::ofstream tea_visit(filename, std::ofstream::app);

  for (int yy = 0; yy < settings.grid_y_chunks; ++yy) {
    for (int xx = 0; xx < settings.grid_x_chunks; ++xx) {
      tea_visit << calc_x_y_time_step_filename(xx, yy, time_step) << std::endl;
    }
  }
  tea_visit.close();
}

void visit_vtk_file(int time_step, Chunk *chunks, Settings &settings) {
  auto chunk = chunks[0];

  int dim_x = chunk.right - chunk.left;
  int dim_y = chunk.top - chunk.bottom;
  int dim_z = 1;

  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  strncpy(filename, settings.tea_vtk_path_name, MAX_CHAR_LEN);
  strcat(filename, calc_x_y_time_step_filename(settings.cart_coords[X_AXIS], settings.cart_coords[Y_AXIS], time_step));
  std::ofstream tea_x_y_ts(filename, std::ofstream::out);

  tea_x_y_ts << "# vtk DataFile Version 5.1" << std::endl;
  tea_x_y_ts << "vtk output" << std::endl;
  tea_x_y_ts << "ASCII" << std::endl;
  tea_x_y_ts << "DATASET RECTILINEAR_GRID" << std::endl;
  tea_x_y_ts << "DIMENSIONS " << dim_x + 1 << " " << dim_y + 1 << " 1" << std::endl;

  tea_x_y_ts << "X_COORDINATES " << dim_x + 1 << " double" << std::endl;
  for (int xx = chunk.left; xx <= chunk.right; ++xx) {
    tea_x_y_ts << std::scientific << std::setw(12) << std::setprecision(4) << settings.grid_x_min + settings.dx * ((double)xx) << " ";
  }
  tea_x_y_ts << std::endl;

  tea_x_y_ts << "Y_COORDINATES " << dim_y + 1 << " double" << std::endl;
  for (int yy = chunk.bottom; yy <= chunk.top; ++yy) {
    tea_x_y_ts << std::scientific << std::setw(12) << std::setprecision(4) << settings.grid_y_min + settings.dy * ((double)yy) << " ";
  }
  tea_x_y_ts << std::endl;

  tea_x_y_ts << "Z_COORDINATES " << dim_z << " double" << std::endl;
  tea_x_y_ts << "0" << std::endl;

  tea_x_y_ts << "CELL_DATA " << dim_x * dim_y << std::endl;
  tea_x_y_ts << "FIELD FieldData 3" << std::endl;

  tea_x_y_ts << "density 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = settings.halo_depth; yy < chunk.y - settings.halo_depth; ++yy) {
    for (int xx = settings.halo_depth; xx < chunk.x - settings.halo_depth; ++xx) {
      int idx = yy * chunk.x + xx;
      //tea_x_y_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.density[idx] << std::endl;
    }
  }

  tea_x_y_ts << "energy 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = settings.halo_depth; yy < chunk.y - settings.halo_depth; ++yy) {
    for (int xx = settings.halo_depth; xx < chunk.x - settings.halo_depth; ++xx) {
      int idx = yy * chunk.x + xx;
      //tea_x_y_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.energy0[idx] << std::endl;
    }
  }

  tea_x_y_ts << "temperature 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = settings.halo_depth; yy < chunk.y - settings.halo_depth; ++yy) {
    for (int xx = settings.halo_depth; xx < chunk.x - settings.halo_depth; ++xx) {
      int idx = yy * chunk.x + xx;
      //tea_x_y_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.u[idx] << std::endl;
    }
  }

  tea_x_y_ts.close();
}