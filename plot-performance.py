import logging

import matplotlib.pyplot as plt

import re

log = logging.getLogger("error-between")
logging.basicConfig(format='%(asctime)s [%(levelname)s] :: %(message)s', datefmt='%m/%d/%Y %I:%M:%S %p',
                    level=logging.INFO)


def subplot(ax, iters, iter_err, err_info, avg_info):
    avg_iter_err = [sum(iter_err) / len(iter_err)] * len(iters)

    ax.xlabel("iteration -->")
    ax.ylabel("temperature error -->")
    ax.plot(iters, iter_err, iters, avg_iter_err, 'k--')
    ax.legend((err_info, avg_info), loc='right', shadow=True)


def plot(err_context, experiment_name, plot_dir):
    log.info("Plotting temperature error analysis...")
    # X axis
    iters = list(err_context['relative'].keys())
    # Y axis
    iter_mre = list(err_context['relative'].values())
    print(f"{iter_mre}")
    avg_iter_mre = [sum(iter_mre) / len(iter_mre)] * len(iters)
    max_abs = list(err_context['max_relative'].values())

    fig, ax1 = plt.subplots()
    plt.title(f"{experiment_name} temperature error analysis")
    color = 'tab:red'
    ax1.set_xlabel("iteration -->")
    ax1.set_ylabel("iteration max relative error", color=color)
    ax1.plot(iters, max_abs, color=color)
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()
    color = 'tab:blue'
    ax2.set_ylabel("avg iteration relative error", color=color)
    ax2.plot(iters, iter_mre, iters, avg_iter_mre, 'k--', color=color)
    ax2.tick_params(axis='y', labelcolor=color)

    plot_filename = os.path.join(plot_dir, experiment_name)
    fig.tight_layout()
    plt.savefig(plot_filename)
    log.info(f"Temperature error analysis plotted in '{plot_filename}'.")


def decode_logs(log_dir):
    log.info("Decoding tea.out...")
    log_filename = os.path.join(log_dir, "tea.out")

    with open(log_filename, 'r') as logfile:
        data = logfile.read()

    timestep_pattern = re.compile(r'Timestep\s+(\d+)', re.MULTILINE)
    cg_pattern = re.compile(r'CG:\s+(\d+)\siterations', re.MULTILINE)
    wallclock_pattern = re.compile(r'Wallclock:\s+([\d\.]+)s', re.MULTILINE)
    ts, cg_iters, clk = timestep_pattern.findall(data), cg_pattern.findall(data), wallclock_pattern.findall(data)
    log.info("tea.out decoded")
    return ts, cg_iters, clk


def main(log_dir, output_dir, experiment_name):
    ts, cg_iters, clk = decode_logs(log_dir)

    ts, cg_iters, clk = [float(x) for x in ts], [float(x) for x in cg_iters], [float(x) for x in clk]

    log.info(f"Plotting performance analysis...")

    # X axis    :: iters
    # Y left    :: time
    # Y right   :: cg iters

    fig, ax1 = plt.subplots()
    plt.title(f"Performance analysis: '{experiment_name}' experiment")
    color = 'tab:blue'
    ax1.set_xlabel("timesteps")
    ax1.set_ylabel("wallclock", color=color)
    ax1.plot(ts, clk, color=color)
    ax1.tick_params(axis='y', labelcolor=color)

    ax2 = ax1.twinx()
    color = 'tab:orange'
    ax2.set_ylabel("solver num. iterations", color=color)
    ax2.plot(ts, cg_iters, color=color)
    ax2.tick_params(axis='y', labelcolor=color)

    plot_filename = os.path.join(output_dir, experiment_name)
    fig.tight_layout()
    plt.savefig(plot_filename)

    log.info(f"Performance analysis plotted in '{plot_filename}'.")


if __name__ == "__main__":
    import argparse
    import os

    parser = argparse.ArgumentParser(
        description="Plot performance in terms of wallclock and solver iterations at each step, given the output logs.")
    parser.add_argument(
        "--log",
        help="the directory containing the tea.out",
        required=True
    )
    parser.add_argument(
        "-o",
        "--output",
        help="the directory to produce the plots in",
        required=True
    )
    parser.add_argument(
        "-e",
        "--experiment-name",
        help="the name of the experiment",
        required=True
    )
    args = parser.parse_args()

    if not os.path.exists(args.log):
        log.error(f"'{args.log}' path does not exist.")
        exit(1)
    if not os.path.exists(args.output):
        os.makedirs(args.output, exist_ok=True)
        log.info(f"'{args.output}' directory created.")

    print(f"-- tea.out directory:\t{args.log}")
    print(f"-- Output directory:\t{args.output}")

    main(args.log, args.output, args.experiment_name)
