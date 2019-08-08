function fold-execute() {
    [[ $TRAVIS == true ]] && echo -en "travis_fold:start:$(echo $@)\r" || true
    echo "--- Executing: $@"
    "$@"
    rcode=$?
    [ $rcode -eq 0 ] || exit $rcode
    [[ $TRAVIS == true ]] && echo -en "travis_fold:end:$(echo $@)\r" || true
}