import vtk
import glob
import numpy
from collections import OrderedDict


def decode(vtk_filenames):
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
    print("... filenames decoded ...")
    return vtk_files_info


def recover(vtk_files_info):
    """Check missing VTK dumps. In case, perform recovery pointing to the last valid VTK file."""
    last_vtk_filename_per_xy = {}

    for iter in vtk_files_info:
        print(f"... checking missing VTK files for iter. no. {iter}")

        xys = {vtk['x.y'] for vtk in vtk_files_info[iter]}
        missing_xys = last_vtk_filename_per_xy.keys() - xys

        for missing_xy in missing_xys:
            vtk_files_info[iter].append({
                'x.y': missing_xy,
                'filename': last_vtk_filename_per_xy[missing_xy]
            })
            if iter != 0:
                print(
                    f">> Missing VTK file for chunk {missing_xy} on iter. no. {iter}: re-using {last_vtk_filename_per_xy[missing_xy]}")

        for vtk in vtk_files_info[iter]:
            last_vtk_filename_per_xy[vtk["x.y"]] = vtk["filename"]

    print("... missing VTK files recovered ...")


def merge(vtk_iter_files):
    # todo :: merge vtk dumps per iteration
    print(vtk_iter_files)


def read_vtk_file(filename):
    reader = vtk.vtkRectilinearGridReader()
    reader.SetFileName(filename)
    reader.Update()
    return reader.GetOutput()


def write_vtk_file(data, field_data, output_filename):
    writer = vtk.vtkRectilinearGridWriter()
    writer.SetFileName(output_filename)
    writer.SetInputData(data)
    writer.SetFieldDataName()
    writer.SetFieldData(field_data)
    writer.Write()


def main(input_dir, output_dir, tea_visit_filename, num_chunks, grid_x_chunks, grid_y_chunks, visit_frequency):
    print(f"-- Input directory:\t{input_dir}")
    print(f"-- Output directory:\t{output_dir}")
    print(f"-- Visit info filename:\t{tea_visit_filename}")
    print(f"-- Num. tot. chunks:\t{num_chunks}")
    print(f"-- Num. X chunks:\t{grid_x_chunks}")
    print(f"-- Num. Y chunks:\t{grid_y_chunks}")
    print(f"-- Visit frequency:\t{visit_frequency}")

    # Get list of VTK files in the input directory
    vtk_filenames = glob.glob(f"{input_dir}/*.vtk")
    if not vtk_filenames:
        print(">> No VTK files found in the specified directory.")
        return

    print()
    print(">> Postprocessing started...")

    vtk_files_info = decode(vtk_filenames)
    recover(vtk_files_info)

    for iter in vtk_files_info:
        merge(vtk_files_info[iter])

    # vtk_per_iter = group_by_iteration(vtk_files)
    # print(vtk_per_iter)

    # Merge VTK files
    # merged_data, merged_field_data = merge_vtk_files(vtk_files)

    # output_filename = os.path.join(output_dir, "output.vtk")
    # Write the merged data to a single VTK file
    # write_vtk_file(merged_data, merged_field_data, output_filename)
    # print(f"Merged VTK file written to {output_filename}")


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(description="Merge multiple VTK files into a single one.")
    parser.add_argument(
        "-i",
        "--input",
        help="The directory containing the input VTK files",
        default="target/vtk"
    )
    parser.add_argument(
        "-o",
        "--output",
        help="The directory for the merged VTK files",
        default="target/vtk/postprocess"
    )
    parser.add_argument(
        "-v",
        "--visit",
        help="The path and name of the 'tea.visit' file",
        default="target/vtk/tea.visit"
    )
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f">> '{args.input}' path does not exist.")
    if not os.path.exists(args.output):
        print(f">> One or more directories in '{args.output}' do not exist.")
        os.makedirs(args.output, exist_ok=True)
        print(f">> Missing directories in '{args.output}' created.")
    if not os.path.isfile(args.visit):
        print(f">> '{args.visit}' file does not exist.")
        exit(1)

    visit_vars = {}
    with open(args.visit, 'r') as tea_visit:
        for line in tea_visit:
            data = line.strip().split()
            if len(data) == 2:
                visit_vars[data[0]] = int(data[1])

    main(args.input, args.output, args.visit, int(visit_vars["num_chunks"]), int(visit_vars["grid_x_chunks"]),
         int(visit_vars["grid_y_chunks"]), int(visit_vars["visit_frequency"]))
