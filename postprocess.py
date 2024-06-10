import glob
from collections import OrderedDict

import vtk


def decode_filename(vtk_filenames):
    """Extract info from VTK filenames and group dumps by iteration."""
    print("... decoding filenames ...")
    vtk_files_info = {}
    for vtk_filename in vtk_filenames:
        # tea . x . y . iter . vtk
        info = vtk_filename.split('.')
        iter = int(info[3])
        x = int(info[1])
        y = int(info[2])

        if iter not in vtk_files_info:
            vtk_files_info[iter] = []
        vtk_files_info[iter].append({
            "x.y": (x, y),
            "filename": vtk_filename
        })
    vtk_files_info = OrderedDict(sorted(vtk_files_info.items()))
    print(">> Filenames decoded.")
    # vtk_files_info : dict [iter] -> {x.y: ... ,  filename: ...}
    return vtk_files_info


def recover_missing_files(vtk_files_info):
    """Check missing VTK dumps. In case, perform recovery pointing to the last valid VTK file."""
    last_vtk_filename_per_xy = {}

    for iter in vtk_files_info:
        print(f"... checking missing VTK files for iter. no. {iter}")

        xys = {vtk["x.y"] for vtk in vtk_files_info[iter]}
        missing_xys = last_vtk_filename_per_xy.keys() - xys

        for missing_xy in missing_xys:
            vtk_files_info[iter].append({
                "x.y": missing_xy,
                "filename""": last_vtk_filename_per_xy[missing_xy]
            })
            if iter != 0:
                print(
                    f">> Missing VTK file for chunk {missing_xy} on iter. no. {iter}: re-using {last_vtk_filename_per_xy[missing_xy]}")

        for vtk in vtk_files_info[iter]:
            last_vtk_filename_per_xy[vtk["x.y"]] = vtk["filename"]

        vtk_files_info[iter] = sorted(vtk_files_info[iter], key=lambda item: item["x.y"])

    print(">> Missing VTK files recovered.")


def merge_vtk_file(iter, num_y_chunks, vtk_iter_files_info):
    """Merge VTK files related to the same iteration."""
    print(f"... merging VTK files for iter. no. {iter}")

    for file_info in vtk_iter_files_info:
        vtk_file = read_vtk_file(file_info["filename"])
        file_info["vtk"] = vtk_file

    x_coords, y_coords, z_coords = merge_coordinates(vtk_iter_files_info)
    density, energy, temperature = merge_cell_data(num_y_chunks, vtk_iter_files_info)
    vtk_out = build_vtk_file(x_coords, y_coords, z_coords, density, energy, temperature)

    print(f">> Merge completed for iter. no. {iter}.")
    return vtk_out


def read_vtk_file(filename) -> vtk.vtkRectilinearGrid:
    """Read a VTK file given the filename."""
    print(f"... reading {filename} ...")
    reader = vtk.vtkRectilinearGridReader()
    reader.SetFileName(filename)
    reader.Update()
    return reader.GetOutput()


def merge_coordinates(vtk_iter_files_info):
    """Given some VTK files for an iteration, merge X, Y and Z coordinates"""
    # vtk_files_info : dict [iter] -> {x.y: ... ,  filename: ... , vtk: ...}

    # Temporary coordinates
    x_coords, y_coords = set(), set()

    for file_info in vtk_iter_files_info:
        vtk_file = file_info["vtk"]

        file_x_coords, file_y_coords = vtk_file.GetXCoordinates(), vtk_file.GetYCoordinates()

        for i in range(file_x_coords.GetNumberOfValues()):
            x_coords.add(file_x_coords.GetValue(i))
        for i in range(file_y_coords.GetNumberOfValues()):
            y_coords.add(file_y_coords.GetValue(i))

    # Order x coordinates
    x_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    for x in sorted(list(x_coords)):
        x_vtk_coords.InsertNextValue(x)

    # Order y coordinates
    y_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    for y in sorted(list(y_coords)):
        y_vtk_coords.InsertNextValue(y)

    # Init z coordinates
    z_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    z_vtk_coords.InsertNextValue(0)

    return x_vtk_coords, y_vtk_coords, z_vtk_coords


def merge_cell_data(num_y_chunks, vtk_iter_files_info):
    """Given some VTK files for an iteration, merge density, energy and temperature cell data"""
    density, energy, temperature = [], [], []

    # Iterate each row of chunks
    for yy in range(num_y_chunks):
        vtk_per_y: list = find_vtk_file_by_y(yy, vtk_iter_files_info)
        num_y_chunk_cells: int = vtk_per_y[0]["vtk"].GetYCoordinates().GetNumberOfValues() - 1

        # Iterate each row of cells for each row of chunks
        for rr in range(num_y_chunk_cells):
            # Fetch in order the line of cells, iterating chunks in the same row
            for vtk_info in vtk_per_y:
                num_x_chunk_cells: int = vtk_info["vtk"].GetXCoordinates().GetNumberOfValues() - 1

                # Retrieve and append #num_x_chunk_cells actual values
                for cc in range(num_x_chunk_cells):
                    idx = rr * num_x_chunk_cells + cc
                    density.append(vtk_info["vtk"].GetCellData().GetArray(0).GetValue(idx))
                    energy.append(vtk_info["vtk"].GetCellData().GetArray(1).GetValue(idx))
                    temperature.append(vtk_info["vtk"].GetCellData().GetArray(2).GetValue(idx))

    vtk_cell_data = vtk.vtkCellData().NewInstance()

    vtk_density = vtk.vtkDoubleArray().NewInstance()
    vtk_density.SetName("density")
    for de in density:
        vtk_density.InsertNextValue(de)

    vtk_energy = vtk.vtkDoubleArray().NewInstance()
    vtk_energy.SetName("energy")
    for en in energy:
        vtk_energy.InsertNextValue(en)

    vtk_temperature = vtk.vtkDoubleArray().NewInstance()
    vtk_temperature.SetName("temperature")
    for te in temperature:
        vtk_temperature.InsertNextValue(te)

    return vtk_density, vtk_energy, vtk_temperature


def find_vtk_file_by_y(yy, vtk_iter_files_info):
    vtk_per_y = []
    for vtk_info in vtk_iter_files_info:
        (x, y) = vtk_info["x.y"]
        if y == yy:
            vtk_per_y.append(vtk_info)
    return vtk_per_y


def find_vtk_file_by_xy(xx, yy, vtk_iter_files_info):
    for vtk_info in vtk_iter_files_info:
        (x, y) = vtk_info["x.y"]
        if (x, y) == (xx, yy):
            return vtk_info
    return None


def build_vtk_file(x_coords, y_coords, z_coords, density, energy, temperature):
    vtk_out = vtk.vtkRectilinearGrid().NewInstance()

    vtk_out.SetDimensions(x_coords.GetNumberOfValues(), y_coords.GetNumberOfValues(), z_coords.GetNumberOfValues())
    vtk_out.SetXCoordinates(x_coords)
    vtk_out.SetYCoordinates(y_coords)
    vtk_out.SetZCoordinates(z_coords)

    vtk_out.GetCellData().AddArray(density)
    vtk_out.GetCellData().AddArray(energy)
    vtk_out.GetCellData().AddArray(temperature)

    return vtk_out


def write_vtk_file(iter, destination, vtk_out, binary_format=False):
    """Writing the VTK file for a given iteration."""
    print(f"... printing the VTK file for the iter. no. {iter} ...")
    writer = vtk.vtkRectilinearGridWriter().NewInstance()
    writer.SetFileName(destination)
    writer.SetInputData(vtk_out)
    if binary_format:
        writer.SetFileTypeToBinary()
    writer.Write()
    print(f">> VTK file printed for iter. no. {iter} as {destination}.")


def main(input_dir, output_dir, output_prefix,  binary_format, grid_y_chunks):
    # Get list of VTK files in the input directory
    vtk_filenames = glob.glob(f"{input_dir}/*.vtk")
    if not vtk_filenames:
        print(">> No VTK files found in the specified directory.")
        return

    print()
    print(">> Postprocessing started...")

    vtk_files_info = decode_filename(vtk_filenames)
    recover_missing_files(vtk_files_info)

    for iter in vtk_files_info:
        vtk_out = merge_vtk_file(iter, grid_y_chunks, vtk_files_info[iter])
        output_filename = output_prefix + "." + str(iter).zfill(5) + ".vtk"
        os.path.join(output_dir, output_filename)
        write_vtk_file(iter, os.path.join(output_dir, output_filename), vtk_out, binary_format)

    print(">> Postprocessing finished successfully.")


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(description="Merge multiple VTK files into a single one.")
    parser.add_argument(
        "-i",
        "--input",
        help="the directory containing the input VTK files",
        default="target/vtk",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="the directory to produce the merged VTK files in",
        default="target/vtk/postprocess"
    )
    parser.add_argument(
        "-p",
        "--output-prefix",
        help="the prefix to introduce to output VTK filenames",
        default="tea"
    )
    parser.add_argument(
        "-v",
        "--visit",
        help="the directory containing the 'tea.visit' file",
        default="target/vtk"
    )
    parser.add_argument(
        "--bin",
        help="whether the VTK files should be generated in a binary format",
        type=bool,
        default=False
    )
    parser.add_argument(
        "--rm",
        help="whether to remove the VTK files in the input directory after postprocessing",
        type=bool,
        default=False
    )
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f">> '{args.input}' path does not exist.")
    if not os.path.exists(args.output):
        print(f"!! One or more directories in '{args.output}' do not exist.")
        os.makedirs(args.output, exist_ok=True)
        print(f">> Missing directories in '{args.output}' created.")
    tea_visit_filename = os.path.join(args.visit, 'tea.visit')
    if not os.path.isfile(tea_visit_filename):
        print(f">> '{tea_visit_filename}' file does not exist.")
        exit(1)

    visit_vars = {}
    with open(tea_visit_filename, 'r') as tea_visit:
        for line in tea_visit:
            data = line.strip().split()
            if len(data) == 2:
                visit_vars[data[0]] = int(data[1])

    print(f"-- Input directory:\t{args.input}")
    print(f"-- Output directory:\t{args.output}")
    print(f"-- Tea.visit:\t\t{tea_visit_filename}")
    print(f"-- Num. Y chunks:\t{visit_vars['grid_y_chunks']}")
    print(f"-- Num. X chunks:\t{visit_vars['grid_x_chunks']}")
    print(f"-- Binary format:\t{args.bin}")
    print(f"-- Remove VTK files after postprocessing:\t{args.rm}")
    main(args.input, args.output, args.output_prefix, args.bin, int(visit_vars["grid_y_chunks"]))

    if args.rm:
        vtk_filenames = glob.glob(f"{args.input}/*.vtk")
        print(f">> Deleting VTK files in {args.input}/*.vtk")
        for vtk_filename in vtk_filenames:
            try:
                os.remove(vtk_filename)
            except Exception as e:
                print(f"!! Error deleting VTK file {vtk_filename}: {e}")
