#!/bin/bash

# The amount of memory we want to be available for hugepages (in MB).
hugepage_mb=1024

# The platform specific hugepage size (in KB).
hugepage_size_kb=$(grep Hugepagesize /proc/meminfo | awk {'print $2'})

# The number of hugepages needed.
nr_hugepages=$(($hugepage_mb/(hugepage_size_kb / 1024)))

echo "Reserving $hugepage_mb MB for hugepages, which is $nr_hugepages hugepages."

sysctl -w vm.nr_hugepages=$nr_hugepages
