#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

pg_ctlcluster 13 main start
su -c "psql  --command \"ALTER USER postgres WITH PASSWORD 'password';\"" - postgres

finish() {
   pg_ctlcluster 13 main stop
}

trap finish EXIT

export PGPORT=${PGPORT-5432}
export PGPASSWORD=password
export PGUSER=postgres
export PGHOST=localhost

$DIR/blockvault_unittests

