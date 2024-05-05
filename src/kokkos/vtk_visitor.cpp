#include "vtk_visitor.h"
#include "comms.h"
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#define MAX_CHAR_LEN 256

void track_all_vtk_files(int time_step, Settings &settings);
void visit_vtk_file(int time_step, Chunk *chunks, Settings &settings);

char *calc_rank_time_step_filename(int rank, int time_step) {
  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  std::ostringstream dynamic_filename;
  dynamic_filename << "tea." << std::setfill('0') << std::setw(5) << rank + 1;
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
  tea_visit << "!NUM_CHUNKS " << settings.num_chunks << std::endl;
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

  for (int rr = 0; rr < settings.num_ranks; ++rr) {
    tea_visit << calc_rank_time_step_filename(rr, time_step) << std::endl;
  }
  tea_visit.close();
}

void visit_vtk_file(int time_step, Chunk *chunks, Settings &settings) {
  auto chunk = chunks[0];

  char *filename = (char *)malloc(sizeof(char) * MAX_CHAR_LEN);
  strncpy(filename, settings.tea_vtk_path_name, MAX_CHAR_LEN);
  strcat(filename, calc_rank_time_step_filename(settings.rank, time_step));
  std::ofstream tea_chunk_ts(filename, std::ofstream::out);

  int dim_x = chunk.right - chunk.left;
  int dim_y = chunk.top - chunk.bottom;
  int dim_z = 1;

  tea_chunk_ts << "# vtk DataFile Version 3.0" << std::endl;
  tea_chunk_ts << "vtk output" << std::endl;
  tea_chunk_ts << "ASCII" << std::endl;
  tea_chunk_ts << "DATASET RECTILINEAR_GRID" << std::endl;
  tea_chunk_ts << "DIMENSIONS " << dim_x << " " << dim_y << " 1" << std::endl;

  tea_chunk_ts << "X_COORDINATES " << dim_x << " double" << std::endl;
  for (float xx = chunk.left; xx < chunk.right; ++xx) {
    tea_chunk_ts << std::scientific << std::setw(12) << std::setprecision(4) << xx << std::endl;
  }

  tea_chunk_ts << "Y_COORDINATES " << dim_y << " double" << std::endl;
  for (float yy = chunk.bottom; yy < chunk.top; ++yy) {
    tea_chunk_ts << std::scientific << std::setw(12) << std::setprecision(4) << yy << std::endl;
  }

  tea_chunk_ts << "Z_COORDINATES " << dim_z << " double" << std::endl;
  tea_chunk_ts << "0" << std::endl;

  tea_chunk_ts << "CELL_DATA " << dim_x * dim_y << std::endl;
  tea_chunk_ts << "FIELD FieldData 3" << std::endl;

  tea_chunk_ts << "density 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = chunk.bottom; yy < chunk.top; ++yy) {
    for (int xx = chunk.left; xx < chunk.right; ++xx) {
      int idx = yy * dim_x + xx;
      tea_chunk_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.density(idx) << std::endl;
    }
  }

  tea_chunk_ts << "energy 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = chunk.bottom; yy < chunk.top; ++yy) {
    for (int xx = chunk.left; xx < chunk.right; ++xx) {
      int idx = yy * dim_x + xx;
      tea_chunk_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.energy0(idx) << std::endl;
    }
  }

  tea_chunk_ts << "temperature 1 " << dim_x * dim_y << " double" << std::endl;
  for (int yy = chunk.bottom; yy < chunk.top; ++yy) {
    for (int xx = chunk.left; xx < chunk.right; ++xx) {
      int idx = yy * dim_x + xx;
      tea_chunk_ts << std::scientific << std::setw(12) << std::setprecision(4) << chunk.u(idx) << std::endl;
    }
  }

  tea_chunk_ts.close();
}