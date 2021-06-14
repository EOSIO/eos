#!/bin/bash

SCRIPT_NAME='test-package.gen.sh'

cat > $SCRIPT_NAME <<EOL
#!/bin/bash
set -eu

if [[ \$(apt-get --version 2>/dev/null) ]]; then # debian family
    UPDATE='apt-get update'
    echo "$ \$UPDATE"
    eval \$UPDATE
    echo '--- :minidisc: Install EOSIO'
    INSTALL="apt-get install -y /eos/*.deb"
    echo "$ \$INSTALL"
    eval \$INSTALL
elif [[ \$(yum --version 2>/dev/null) ]]; then # RHEL family
    UPDATE='yum check-update || :'
    echo "$ \$UPDATE"
    eval \$UPDATE
    INSTALL="yum install -y /eos/*.rpm"
    echo "$ \$INSTALL"
    eval \$INSTALL
else
    echo 'ERROR: Package manager not detected!'
    exit 3
fi

nodeos --full-version
EOL

chmod +x $SCRIPT_NAME

docker pull $IMAGE
docker run --rm -v "$(pwd):/eos" -w '/eos' -it $IMAGE ./$SCRIPT_NAME
