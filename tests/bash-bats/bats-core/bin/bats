#!/usr/bin/env bash

set -e

BATS_READLINK='true'
if command -v 'greadlink' >/dev/null; then
  BATS_READLINK='greadlink'
elif command -v 'readlink' >/dev/null; then
  BATS_READLINK='readlink'
fi

bats_resolve_absolute_root_dir() {
  local cwd="$PWD"
  local path="$1"
  local result="$2"
  local target_dir
  local target_name
  local original_shell_options="$-"

  # Resolve the parent directory, e.g. /bin => /usr/bin on CentOS (#113).
  set -P

  while true; do
    target_dir="${path%/*}"
    target_name="${path##*/}"

    if [[ "$target_dir" != "$path" ]]; then
      cd "$target_dir"
    fi

    if [[ -L "$target_name" ]]; then
      path="$("$BATS_READLINK" "$target_name")"
    else
      printf -v "$result" -- '%s' "${PWD%/*}"
      set +P "-$original_shell_options"
      cd "$cwd"
      return
    fi
  done
}

export BATS_ROOT
bats_resolve_absolute_root_dir "$0" 'BATS_ROOT'
exec "$BATS_ROOT/libexec/bats-core/bats" "$@"
