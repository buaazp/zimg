#!/bin/bash
top -p `pgrep main |xargs perl -e "print join ',',@ARGV"`
