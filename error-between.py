import glob
import logging

import vtk

log = logging.getLogger("postprocess")
logging.basicConfig(format='%(asctime)s [%(levelname)s] :: %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p',
                    level=logging.INFO)


def decode_filename(vtk_filenames):
    """Extract info from VTK filenames and group dumps by x,y,iteration."""
    log.debug("... decoding filenames ...")
    vtk_files_info = {}
    for vtk_filename in vtk_filenames:
        # tea . iter . vtk
        info = vtk_filename.split('.')
        iter = int(info[1])

        vtk_files_info[iter] = {
            "filename": vtk_filename
        }

    # vtk_files_info : dict [iter] -> {filename: ...}
    return vtk_files_info


def compute_error(iter, vtk_exp_filename, vtk_act_filename):
    """Compute absolute error between two VTK file describing the same iteration"""
    log.debug(f"... computing error for VTK files of iter. no. {iter}")

    vtk_exp_file = read_vtk_file(vtk_exp_filename)
    vtk_act_file = read_vtk_file(vtk_act_filename)

    x_coords, y_coords, z_coords = get_vtk_coordinates(vtk_exp_file)
    density, energy, temperature = error_on_cell_data(vtk_exp_file, vtk_act_file)
    vtk_out = build_vtk_file(x_coords, y_coords, z_coords, density, energy, temperature)
    return vtk_out


def read_vtk_file(filename) -> vtk.vtkRectilinearGrid:
    """Read a VTK file given the filename."""
    log.debug(f"... reading {filename} ...")
    reader = vtk.vtkRectilinearGridReader()
    reader.SetFileName(filename)
    reader.Update()
    return reader.GetOutput()


def get_vtk_coordinates(vtk_iter_file):
    """Given a VTK file for an iteration, get X, Y and Z coordinates"""
    # Temporary coordinates
    x_coords, y_coords = [], []

    file_x_coords, file_y_coords = vtk_iter_file.GetXCoordinates(), vtk_iter_file.GetYCoordinates()

    for i in range(file_x_coords.GetNumberOfValues()):
        x_coords.append(file_x_coords.GetValue(i))
    for i in range(file_y_coords.GetNumberOfValues()):
        y_coords.append(file_y_coords.GetValue(i))

    # Order x coordinates
    x_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    for x in x_coords:
        x_vtk_coords.InsertNextValue(x)

    # Order y coordinates
    y_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    for y in y_coords:
        y_vtk_coords.InsertNextValue(y)

    # Init z coordinates
    z_vtk_coords = vtk.vtkDoubleArray().NewInstance()
    z_vtk_coords.InsertNextValue(0)

    return x_vtk_coords, y_vtk_coords, z_vtk_coords


def error_on_cell_data(vtk_iter_exp_file, vtk_iter_act_file):
    """Given some VTK files for an iteration, merge density, energy and temperature cell data"""
    density, energy, temperature = [], [], []

    num_x_cells = vtk_iter_exp_file.GetXCoordinates().GetNumberOfValues() - 1
    num_y_cells = vtk_iter_exp_file.GetYCoordinates().GetNumberOfValues() - 1
    num_cells = num_x_cells * num_y_cells

    for ii in range(num_cells):
        density.append(
            abs(vtk_iter_exp_file.GetCellData().GetArray(0).GetValue(ii) - vtk_iter_act_file.GetCellData().GetArray(
                0).GetValue(ii))
        )
        energy.append(
            abs(vtk_iter_exp_file.GetCellData().GetArray(1).GetValue(ii) - vtk_iter_act_file.GetCellData().GetArray(
                1).GetValue(ii))
        )
        temperature.append(
            abs(vtk_iter_exp_file.GetCellData().GetArray(2).GetValue(ii) - vtk_iter_act_file.GetCellData().GetArray(
                2).GetValue(ii))
        )

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
    log.debug(f"... printing the VTK file for the iter. no. {iter} ...")
    writer = vtk.vtkRectilinearGridWriter().NewInstance()
    writer.SetFileName(destination)
    writer.SetInputData(vtk_out)
    if binary_format:
        writer.SetFileTypeToBinary()
    writer.Write()
    log.debug(f"VTK file printed for iter. no. {iter} as {destination}.")


def main(expected_dir, actual_dir, output_dir, output_prefix, binary_format):
    # Get list of VTK files in the input directory
    vtk_exp_filenames = glob.glob(f"{expected_dir}/*.vtk")
    if not vtk_exp_filenames:
        log.info(f"No VTK files found in the specified directory: {expected_dir}")
        return

    vtk_act_filenames = glob.glob(f"{actual_dir}/*.vtk")
    if not vtk_act_filenames:
        log.info(f"No VTK files found in the specified directory:{actual_dir}")
        return

    log.info("Error computation started...")

    vtk_exp_files_info = decode_filename(vtk_exp_filenames)
    vtk_act_files_info = decode_filename(vtk_act_filenames)
    log.info("Filenames decoded.")

    for iter in vtk_exp_files_info.keys():
        vtk_exp_filename = vtk_exp_files_info[iter]["filename"]
        vtk_act_filename = vtk_act_files_info[iter]["filename"]
        vtk_out = compute_error(iter, vtk_exp_filename, vtk_act_filename)
        output_filename = output_prefix + "." + str(iter).zfill(5) + ".vtk"
        os.path.join(output_dir, output_filename)
        write_vtk_file(iter, os.path.join(output_dir, output_filename), vtk_out, binary_format)

    log.info("Error computation finished successfully.")


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(
        description="Produce VTK files showing the absolute error between two input VTK files.")
    parser.add_argument(
        "-e",
        "--expected",
        help="the directory containing the reference input VTK files",
        required=True
    )
    parser.add_argument(
        "-a",
        "--actual",
        help="the directory containing the actual input VTK files",
        required=True
    )
    parser.add_argument(
        "-p",
        "--output-prefix",
        help="the prefix to introduce to output VTK filenames",
        default="tea-error"
    )
    parser.add_argument(
        "--bin",
        help="whether the VTK files should be generated in a binary format",
        type=bool,
        default=False
    )
    args = parser.parse_args()

    if not os.path.exists(args.expected):
        log.error(f"'{args.expected}' path does not exist.")
        exit(1)
    if not os.path.exists(args.actual):
        log.error(f"'{args.actual}' path does not exist.")
        exit(1)
    args.output = args.actual + "/error"
    if not os.path.exists(args.output):
        os.makedirs(args.output, exist_ok=True)
        log.info(f"'{args.output}' directory created.")

    print(f"-- Expected values' files directory:\t{args.expected}")
    print(f"-- Actual values' files directory:\t{args.actual}")
    print(f"-- Output directory:\t{args.output}")
    print(f"-- Binary format:\t{args.bin}")

    main(args.expected, args.actual, args.output, args.output_prefix, args.bin)
