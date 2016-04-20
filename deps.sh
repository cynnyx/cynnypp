#!/usr/bin/env bash

#
# google docet - jump within the directory which contains this script
#

SOURCE="${BASH_SOURCE[0]}"

while [ -h "$SOURCE" ]; do
    DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"
    SOURCE="$(readlink "$SOURCE")"
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done

#
# set aside the base dir for future references
#

DIR="$(cd -P "$(dirname "$SOURCE")" && pwd)"

#
# init/update submodules
#

cd "$DIR"
git submodule update --init # --recursive

#
# go back home
#

cd "$DIR"
