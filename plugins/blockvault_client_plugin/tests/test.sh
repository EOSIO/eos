#!/bin/bash

postgres_control=@CMAKE_BINARY_DIR@/scripts/postgres_control.sh
$postgres_control start 
finish() {
   $postgres_control stop
}

trap finish EXIT

export PGPORT=${PGPORT-5432}
export PGPASSWORD=password
export PGUSER=postgres
export PGHOST=localhost
@CMAKE_CURRENT_BINARY_DIR@/blockvault_unittests

