#pragma once

#include "chunk.h"

void init(Settings &settings);
void visit(int time_step, Chunk *chunks, Settings &settings);