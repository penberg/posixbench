#!/usr/bin/env python3

import matplotlib.pyplot as plt
import matplotlib as mpl

import pandas as pd
import itertools
import argparse
import os

parser = argparse.ArgumentParser(description='Generate some plots.')
parser.add_argument("filename", help='The name of a data file to generate a plot from.')
args = parser.parse_args()

df = pd.read_csv(args.filename, delimiter=',', header=0)
df = df.loc[df['percentile'] <= 99]
df['time'] = df['time'] / 1000

plt.style.use('seaborn-ticks')

color = itertools.cycle(mpl.rcParams['axes.prop_cycle'].by_key()['color'])

# Intel Xeon L3 access latency
# (Source: https://www.anandtech.com/show/8423/intel-xeon-e5-version-3-up-to-18-haswell-ep-cores-/11)
L3_access_latency = 0.020 # microseconds 

# Intel Optane random load
# (Source: https://arxiv.org/pdf/1903.05714.pdf)
optane_latency = 0.305 # microseconds

fig, ax = plt.subplots()
for name, group in df.groupby('scenario'):
  group.plot(x='time', y='percentile', legend=True, label=name, ax=ax)

plt.axvline(x=L3_access_latency, label="L3 cache", color="black", linestyle="dotted")
plt.axvline(x=optane_latency, label="Optane (random load)", color="black", linestyle="dashed")

plt.xlabel('Time (μs)')
plt.ylabel('Percentile (%)')
plt.legend(loc='lower right', frameon=True, framealpha=1)

prefix, _ = os.path.splitext(args.filename)
plt.savefig("%s.pdf" % (prefix), format='pdf')
plt.savefig("%s.png" % (prefix), format='png')
