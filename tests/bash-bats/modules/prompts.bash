#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing Prompts" {
    # All yes pass
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION"
    [[ ! -z $(echo "${output}" | grep "EOSIO has been successfully built") ]] || exit
    # First no shows "aborting"  
    run bash -c "printf \"n\n%.0s\" {1..2} | ./$SCRIPT_LOCATION -P"
    [[ "${output##*$'\n'}" =~ .*User.aborted.* ]] || exit
    # # Mongo
    if [[ $NAME == "Amazon Linux" ]] || [[ $ARCH == "Darwin" ]]; then
        run bash -c "printf \"n\nn\nn\nn\n\" | ./$SCRIPT_LOCATION -m"
    else
        run bash -c "printf \"y\nn\nn\nn\n\" | ./$SCRIPT_LOCATION -m"
    fi
    [[ ! -z $(echo "${output}" | grep "Existing MongoDB will be used") ]] || exit
}
