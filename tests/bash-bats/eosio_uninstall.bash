#!/usr/bin/env bats
load test_helper

SCRIPT_LOCATION="scripts/eosio_uninstall.bash"
TEST_LABEL="[eosio_uninstall]"

mkdir -p $SRC_LOCATION
mkdir -p $OPT_LOCATION
mkdir -p $VAR_LOCATION
mkdir -p $BIN_LOCATION
mkdir -p $VAR_LOCATION/log
mkdir -p $ETC_LOCATION
mkdir -p $LIB_LOCATION
mkdir -p $MONGODB_LOG_LOCATION
mkdir -p $MONGODB_DATA_LOCATION

# A helper function is available to show output and status: `debug`

@test "${TEST_LABEL} > Usage is visible with right interaction" {
  run ./$SCRIPT_LOCATION -help
  [[ $output =~ "Usage ---" ]] || exit
  run ./$SCRIPT_LOCATION --help
  [[ $output =~ "Usage ---" ]] || exit
  run ./$SCRIPT_LOCATION help
  [[ $output =~ "Usage ---" ]] || exit
  run ./$SCRIPT_LOCATION blah
  [[ $output =~ "Usage ---" ]] || exit
}

@test "${TEST_LABEL} > Testing user prompts" {
  ## No y or no warning and re-prompt
  run bash -c "echo -e \"\nx\nx\nx\" | ./$SCRIPT_LOCATION"
  ( [[ "${lines[${#lines[@]}-1]}" == "Please type 'y' for yes or 'n' for no." ]] && [[ "${lines[${#lines[@]}-2]}" == "Please type 'y' for yes or 'n' for no." ]] ) || exit
  ## All yes pass
  run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
  [[ "${output##*$'\n'}" == "[EOSIO Removal Complete]" ]] || exit
  ## First no shows "Cancelled..."
  run bash -c "echo \"n\" | ./$SCRIPT_LOCATION"
  [[ "${output##*$'\n'}" =~ "Cancelled EOSIO Removal!" ]] || exit
  ## What would you like to do?"
  run bash -c "echo \"\" | ./$SCRIPT_LOCATION"
  [[ "${output##*$'\n'}" =~ "What would you like to do?" ]] || exit
}

@test "${TEST_LABEL} > Testing executions" {
  run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
  ### Make sure deps are loaded properly
  [[ "${output}" =~ "Executing: rm -rf" ]] || exit
  [[ $ARCH == "Darwin" ]] && ( [[ "${output}" =~ "Executing: brew uninstall cmake --force" ]] || exit )
  # Legacy support
  [[ ! -z $( echo $output | grep "Executing: rmdir ${HOME}/src") ]] || exit
}


@test "${TEST_LABEL} > --force" {
  run ./$SCRIPT_LOCATION --force
  # Make sure we reach the end
  [[ "${output##*$'\n'}" == "[EOSIO Removal Complete]" ]] || exit
}

@test "${TEST_LABEL} > --force + --full" {
  run ./$SCRIPT_LOCATION --force --full
  ([[ ! "${output[*]}" =~ "Library/Application\ Support/eosio" ]] && [[ ! "${output[*]}" =~ ".local/share/eosio" ]]) && exit
  [[ "${output##*$'\n'}" == "[EOSIO Removal Complete]" ]] || exit
}

rm -rf $SRC_LOCATION
rm -rf $OPT_LOCATION
rm -rf $VAR_LOCATION
rm -rf $BIN_LOCATION
rm -rf $VAR_LOCATION/log
rm -rf $ETC_LOCATION
rm -rf $LIB_LOCATION
rm -rf $MONGODB_LOG_LOCATION
rm -rf $MONGODB_DATA_LOCATION