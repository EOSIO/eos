#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > MongoDB" {
    # Existing MongoDB
    if [[ $NAME == "CentOS Linux" ]] || [[ $NAME == "Amazon Linux" ]]; then
        run bash -c "printf \"y\ny\nn\ny\ny\ny\n\" | ./$SCRIPT_LOCATION -m -P" # which prompt requires first y
    else
        run bash -c "printf \"y\nn\nn\ny\ny\n\" | ./$SCRIPT_LOCATION -m -P"
    fi
    [[ ! -z $(echo "${output}" | grep "Existing MongoDB will be used") ]] || exit
    [[ -z $(echo "${output}" | grep "Ensuring MongoDB installation") ]] || exit
    # Installing ours
    run bash -c "printf \"y\ny\ny\ny\ny\ny\n\" | ./$SCRIPT_LOCATION -m -P"
    [[ -z $(echo "${output}" | grep "Existing MongoDB will be used") ]] || exit
    [[ ! -z $(echo "${output}" | grep "Ensuring MongoDB installation") ]] || exit
}
