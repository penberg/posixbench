#!/usr/bin/env python3

import matplotlib.pyplot as plt
import matplotlib as mpl

import pandas as pd
import numpy as np
import itertools
import argparse
import os

parser = argparse.ArgumentParser(description='Generate some plots.')
parser.add_argument("filename", help='The name of a data file to generate a plot from.')
args = parser.parse_args()

df = pd.read_csv(args.filename, delimiter=',', header=0)

plt.style.use('seaborn-ticks')

fig, ax = plt.subplots()

labels = []
pkg_energy_means = []
pkg_energy_errs = []
dram_energy_means = []
dram_energy_errs = []

for benchmark_name, benchmark_group in df.groupby('Benchmark', sort=False):
  for scenario_name, scenario_group in df.groupby('Scenario', sort=False):
    labels += [scenario_name]

    pkg_energy = scenario_group['PackageEnergyPerOperation(nJ)']
    pkg_energy_means += [float(pkg_energy.mean() / 1000.0)]
    pkg_energy_errs += [float(pkg_energy.sem() / 1000.0)]

    dram_energy = scenario_group['DRAMEnergyPerOperation(nJ)']
    dram_energy_means += [float(dram_energy.mean() / 1000.0)]
    dram_energy_errs += [float(dram_energy.sem() / 1000.0)]

idx = np.arange(len(pkg_energy_means))

width = 0.35

p1 = plt.bar(idx, pkg_energy_means, width, yerr=pkg_energy_errs)
p2 = plt.bar(idx, dram_energy_means, width, bottom=pkg_energy_means, yerr=dram_energy_errs)
plt.xticks(idx, labels)

plt.legend((p1[0], p2[0]), ('Package', 'DRAM'))

prefix, _ = os.path.splitext(args.filename)

plt.xlabel(f"Benchmark ({prefix})")
plt.ylabel('Energy per operation (mJ)')

plt.savefig("%s.pdf" % (prefix), format='pdf')
plt.savefig("%s.png" % (prefix), format='png')
