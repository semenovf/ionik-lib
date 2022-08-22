#!/bin/bash

if [ -e .git ] ; then

    git checkout master && git pull origin master \
        && git submodule update --init --recursive \
        && git submodule update --init --recursive --remote -- 3rdparty/portable-target \
        && git submodule update --init --recursive --remote -- 3rdparty/pfs/common

fi

