#!/bin/bash
top -pid `pgrep main |xargs perl -e "print join ',',@ARGV"`
