#!/usr/bin/env bats
load ../helpers/functions

@test "${TEST_LABEL} > Testing root user run" {
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P -i /NEWPATH"
    [[ ! -z $(echo "${output}" | grep "User: $(whoami)") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        [[ -z $(echo "${output}" | grep "$SUDO_LOCATION -E") ]] || exit
    fi
    export CURRENT_USER=test
    run bash -c "printf \"y\nn\n\" | ./$SCRIPT_LOCATION -P"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        [[ ! -z $(echo "${output}" | grep "Please install the 'sudo' command before proceeding") ]] || exit
    fi
    install-package sudo WETRUN
    export SUDO_LOCATION=$( command -v sudo )
    run bash -c "printf \"y\n%.0s\" {1..100} | ./$SCRIPT_LOCATION -P -i /NEWPATH"
    [[ ! -z $(echo "${output}" | grep "User: test") ]] || exit
    if [[ $ARCH == "Linux" ]]; then
        [[ ! -z $(echo "${output}" | grep "$SUDO_LOCATION -E .* install -y .*") ]] || exit
    fi

}