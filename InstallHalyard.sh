#!/usr/bin/env bash

set -e

function process_args() {
  while [ "$#" -gt "0" ]
  do
    local key="$1"
    shift
    case $key in
      --user)
        echo "user"
        HAL_USER="$1"
        shift
        ;;
      --version)
        echo "version"
        HALYARD_VERSION="$1"
        shift
        ;;
      -y)
        echo "non-interactive"
        YES=true
        ;;
      --help|-help|-h)
        print_usage
        exit 13
        ;;
      *)
        echo "ERROR: Unknown argument '$key'"
        exit -1
    esac
  done
}

function get_user() {
  local user 

  user=$(who -m | awk '{print $1;}')
  if [ -z "$YES" ]; then
    if [ "$user" = "root" ] || [ -z "$user" ]; then
      read -p "Please supply a non-root user to run Halyard as: " user
    fi
  fi

  echo $user
}

function configure_defaults() {
  if [ -z "$HAL_USER" ]; then
    HAL_USER=$(get_user)
  fi

  if [ -z "$HAL_USER" ]; then
    >&2 echo "You have not supplied a user to run Halyard as."
    exit 1
  fi

  if [ "$HAL_USER" = "root" ]; then
    >&2 echo "Halyard may not be run as root. Supply a user to run Halyard as: "
    >&2 echo "  sudo bash $0 --user <user>"
    exit 1
  fi

  set +e
  id $HAL_USER &> /dev/null

  if [ "$?" != "0" ]; then
    >&2 echo "Supplied user $HAL_USER does not exist"
    exit 1
  fi
  set -e

  if [ -z "$HALYARD_VERSION" ]; then
    HALYARD_VERSION="stable"
  fi

  echo "$(tput bold)Halyard version will be $HALYARD_VERSION $(tput sgr0)"

  home="/Users/$HAL_USER"
  local halconfig_dir="$home/.hal"

  echo "$(tput bold)Halconfig will be stored at $halconfig_dir/config$(tput sgr0)"

  mkdir -p $halconfig_dir
  chown $HAL_USER $halconfig_dir

  mkdir -p /opt/spinnaker/config
  chmod +rx /opt/spinnaker/config

  cat > /opt/spinnaker/config/halyard.yml <<EOL
halyard:
  halconfig:
    directory: $halconfig_dir

spinnaker:
  artifacts:
    debianRepository: $SPINNAKER_REPOSITORY_URL
    dockerRegistry: $SPINNAKER_DOCKER_REGISTRY
    googleImageProject: $SPINNAKER_GCE_PROJECT
EOL

  echo $HAL_USER > /opt/spinnaker/config/halyard-user

  cat > $halconfig_dir/uninstall.sh <<EOL
#!/usr/bin/env bash

if [[ \`/usr/bin/id -u\` -ne 0 ]]; then
  echo "$0 must be executed with root permissions; exiting"
  exit 1
fi

read -p "This script uninstalls Halyard and deletes all of its artifacts, are you sure you want to continue? (y/N): " yes

if [ "\$yes" != "y" ] && [ "\$yes" != "Y" ]; then
  echo "Aborted"
  exit 0
fi

rm -rf /opt/halyard
rm -rf /var/log/spinnaker/halyard

echo "Deleting halconfig and artifacts"
rm -rf /opt/spinnaker/config/halyard*
rm -rf $halconfig_dir 
EOL

  chmod +x $halconfig_dir/uninstall.sh
  echo "$(tput bold)Uninstall script is located at $halconfig_dir/uninstall.sh$(tput sgr0)"
}


function print_usage() {
  cat <<EOF
usage: $0 [-y] [--version=<version>] [--user=<user>]
    -y                              Accept all default options during install
                                    (non-interactive mode).

    --version <version>             Specify the exact verison of Halyard to
                                    install.

    --user <user>                   Specify the user to run Halyard as. This
                                    user must exist.
EOF
}

function install_java() {
  set +e
  local java_version=$(java -version 2>&1 head -1)
  set -e

  if [[ "$java_version" == *"1.8"* ]] || \
     [[ "$java_version" == *"9.0"* ]] || \
     [[ "$java_version" == *"10.0"* ]] || \
     [[ "$java_version" == *"11"* ]] || \
     [[ "$java_version" == "java version \"10\""* ]]; then
    echo "Java is already installed & at the right version"
    return 0;
  fi

  >&2 echo "Java >=8 not yet installed - please install java >=8."
  >&2 echo "  Try using brew or jenv"
  exit 1
}

function install_halyard() {
  TEMPDIR=$(mktemp -d installhalyard.XXXX)
  pushd $TEMPDIR

  curl -OL https://storage.googleapis.com/spinnaker-artifacts/halyard/$HALYARD_VERSION/macos/halyard.tar.gz
  tar -xvf halyard.tar.gz -C /opt

  chown $HAL_USER /opt/halyard

  mv /opt/hal /usr/local/bin
  chmod +x /usr/local/bin/hal

  if [ -f /opt/update-halyard ]; then
    mv /opt/update-halyard /usr/local/bin
    chmod +x /usr/local/bin/update-halyard
  else 
    echo "No update script supplied with installer..."
  fi

  mkdir -p /var/log/spinnaker/halyard
  chown $HAL_USER /var/log/spinnaker/halyard
  chmod 755 /var/log/spinnaker/halyard

  popd
  rm -rf $TEMPDIR
}

process_args $@
configure_defaults

install_java
install_halyard

su $HAL_USER -l -c "hal -v" -s /bin/bash
