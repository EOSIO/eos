# Ensures passed in version values are supported.
function check-version-numbers() {
  CHECK_VERSION_MAJOR=$1
  CHECK_VERSION_MINOR=$2

  if [[ $CHECK_VERSION_MAJOR -lt $EOSIO_MIN_VERSION_MAJOR ]]; then
    exit 1
  fi
  if [[ $CHECK_VERSION_MAJOR -gt $EOSIO_MAX_VERSION_MAJOR ]]; then
    exit 1
  fi
  if [[ $CHECK_VERSION_MAJOR -eq $EOSIO_MIN_VERSION_MAJOR ]]; then
    if [[ $CHECK_VERSION_MINOR -lt $EOSIO_MIN_VERSION_MINOR ]]; then
      exit 1
    fi
  fi
  if [[ $CHECK_VERSION_MAJOR -eq $EOSIO_MAX_VERSION_MAJOR ]]; then
    if [[ $CHECK_VERSION_MINOR -gt $EOSIO_MAX_VERSION_MINOR ]]; then
      exit 1
    fi
  fi
  exit 0
}


# Handles choosing which EOSIO directory to select when the default location is used.
function default-eosio-directories() {
  REGEX='^[0-9]+([.][0-9]+)?$'
  ALL_EOSIO_SUBDIRS=()
  if [[ -d ${HOME}/eosio ]]; then
    ALL_EOSIO_SUBDIRS=($(ls ${HOME}/eosio | sort -V))
  fi
  for ITEM in "${ALL_EOSIO_SUBDIRS[@]}"; do
    if [[ "$ITEM" =~ $REGEX ]]; then
      DIR_MAJOR=$(echo $ITEM | cut -f1 -d '.')
      DIR_MINOR=$(echo $ITEM | cut -f2 -d '.')
      if $(check-version-numbers $DIR_MAJOR $DIR_MINOR); then
        PROMPT_EOSIO_DIRS+=($ITEM)
      fi
    fi
  done
  for ITEM in "${PROMPT_EOSIO_DIRS[@]}"; do
    if [[ "$ITEM" =~ $REGEX ]]; then
      EOSIO_VERSION=$ITEM
    fi
  done
}


# Prompts or sets default behavior for choosing EOSIO directory.
function eosio-directory-prompt() {
  if [[ -z $EOSIO_DIR_PROMPT ]]; then
    default-eosio-directories;
    echo 'No EOSIO location was specified.'
    while true; do
      if [[ $NONINTERACTIVE != true ]]; then
        if [[ -z $EOSIO_VERSION ]]; then
          echo "No default EOSIO installations detected..."
          PROCEED=n
        else
          printf "Is EOSIO installed in the default location: $HOME/eosio/$EOSIO_VERSION (y/n)" && read -p " " PROCEED
        fi
      fi
      echo ""
      case $PROCEED in
        "" )
          echo "Is EOSIO installed in the default location?";;
        0 | true | [Yy]* )
          break;;
        1 | false | [Nn]* )
          if [[ $PROMPT_EOSIO_DIRS ]]; then
            echo "Found these compatible EOSIO versions in the default location."
            printf "$HOME/eosio/%s\n" "${PROMPT_EOSIO_DIRS[@]}"
          fi
          printf "Enter the installation location of EOSIO:" && read -e -p " " EOSIO_DIR_PROMPT;
          EOSIO_DIR_PROMPT="${EOSIO_DIR_PROMPT/#\~/$HOME}"
          break;;
        * )
          echo "Please type 'y' for yes or 'n' for no.";;
      esac
    done
  fi
  export EOSIO_INSTALL_DIR="${EOSIO_DIR_PROMPT:-${HOME}/eosio/${EOSIO_VERSION}}"
}


# Prompts or default behavior for choosing EOSIO.CDT directory.
function cdt-directory-prompt() {
  if [[ -z $CDT_DIR_PROMPT ]]; then
    echo 'No EOSIO.CDT location was specified.'
    while true; do
      if [[ $NONINTERACTIVE != true ]]; then
        printf "Is EOSIO.CDT installed in the default location? /usr/local/eosio.cdt (y/n)" && read -p " " PROCEED
      fi
      echo ""
      case $PROCEED in
        "" )
          echo "Is EOSIO.CDT installed in the default location?";;
        0 | true | [Yy]* )
          break;;
        1 | false | [Nn]* )
          printf "Enter the installation location of EOSIO.CDT:" && read -e -p " " CDT_DIR_PROMPT;
          CDT_DIR_PROMPT="${CDT_DIR_PROMPT/#\~/$HOME}"
          break;;
        * )
          echo "Please type 'y' for yes or 'n' for no.";;
      esac
    done
  fi
  export CDT_INSTALL_DIR="${CDT_DIR_PROMPT:-/usr/local/eosio.cdt}"
}


# Ensures EOSIO is installed and compatible via version listed in tests/CMakeLists.txt.
function nodeos-version-check() {
  INSTALLED_VERSION=$(echo $($EOSIO_INSTALL_DIR/bin/nodeos --version))
  INSTALLED_VERSION_MAJOR=$(echo $INSTALLED_VERSION | cut -f1 -d '.' | sed 's/v//g')
  INSTALLED_VERSION_MINOR=$(echo $INSTALLED_VERSION | cut -f2 -d '.' | sed 's/v//g')

  if [[ -z $INSTALLED_VERSION_MAJOR || -z $INSTALLED_VERSION_MINOR ]]; then
    echo "Could not determine EOSIO version. Exiting..."
    exit 1;
  fi

  if $(check-version-numbers $INSTALLED_VERSION_MAJOR $INSTALLED_VERSION_MINOR); then
    if [[ $INSTALLED_VERSION_MAJOR -gt $EOSIO_SOFT_MAX_MAJOR ]]; then
      echo "Detected EOSIO version is greater than recommended soft max: $EOSIO_SOFT_MAX_MAJOR.$EOSIO_SOFT_MAX_MINOR. Proceed with caution."
    fi
    if [[ $INSTALLED_VERSION_MAJOR -eq $EOSIO_SOFT_MAX_MAJOR && $INSTALLED_VERSION_MINOR -gt $EOSIO_SOFT_MAX_MINOR ]]; then
      echo "Detected EOSIO version is greater than recommended soft max: $EOSIO_SOFT_MAX_MAJOR.$EOSIO_SOFT_MAX_MINOR. Proceed with caution."
    fi
  else
    echo "Supported versions are: $EOSIO_MIN_VERSION_MAJOR.$EOSIO_MIN_VERSION_MINOR - $EOSIO_MAX_VERSION_MAJOR.$EOSIO_MAX_VERSION_MINOR"
    echo "Invalid EOSIO installation. Exiting..."
    exit 1;
  fi
}
