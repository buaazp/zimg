#!/bin/bash
top -pid `pgrep magick_read_image |xargs perl -e "print join ',',@ARGV"`
