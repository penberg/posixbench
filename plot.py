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
df = df.loc[df['threadspercpu'] == 1]
df['scenario'] = df[['cpus', 'threadspercpu']].apply(lambda x: "%2d CPUs" % (x[0]), axis=1)
df['time'] = df['time'] / 1000

plt.style.use('seaborn-ticks')

color = itertools.cycle(mpl.rcParams['axes.prop_cycle'].by_key()['color'])

fig, ax = plt.subplots()
for name, group in df.groupby('scenario'):
  group.plot(x='time', y='percentile', legend=True, label=name, ax=ax)
plt.xlabel('Time (μs)')
plt.ylabel('Percentile (%)')

prefix, _ = os.path.splitext(args.filename)
plt.savefig("%s.pdf" % (prefix), format='pdf')
plt.savefig("%s.png" % (prefix), format='png')
