#!/bin/bash
#usage: test binary part len

dd if=/dev/urandom of=random.bin bs=1024 count=$3
$1 --part $2 --write random.bin
