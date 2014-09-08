#!/bin/bash
find . -name '*.lua' |xargs wc -l
find . -name '*.c' |xargs wc -l
find . -name '*.h' |xargs wc -l
