#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

CONTAINER_ID=`docker run  -p 5432:5432 -e POSTGRES_PASSWORD=password -d postgres`

finish() {
   docker rm -f $CONTAINER_ID
}

trap finish EXIT

export PGPORT=${PGPORT-5432}
export PGPASSWORD=password
export PGUSER=postgres
export PGHOST=localhost

# wait until postgres is ready
until docker run \
  --rm \
  --link $CONTAINER_ID:pg \
  postgres pg_isready \
    -U postgres \
    -h pg; do sleep 1; done

$DIR/blockvault_unittests

