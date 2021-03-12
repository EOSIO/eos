#!/bin/bash

function parse-args() {
while [[ $# > 0 ]]
do
   case "$1" in
      --days|-d)
      DAYS=${2}
      shift
      ;;
      --CA-org|-o)
      CA_ORG=${2}
      ;;
      --CA-CN|-n)
      CA_CN=${2}
      shift
      ;;
      --org-mask|-m)
      ORG_MASK=${2}
      shift
      ;;
      --cn-mask|-cm)
      CN_MASK=${2}
      shift
      ;;
      --group-size|-s)
      GROUP_SIZE=${2}
      shift
      ;;
  esac
  shift
done
}

if [[ $1 == "--help" ]]
then
   echo "Usage:"
   echo "--days:       Number of days for certificate to expire"
   echo "--CA-org:     Certificate Authority organization name"
   echo "--CA-CN:      Certificate Authority common name"
   echo "--org-mask:   Paritipant certificates name mask in format of name{number}"
   echo "--cn-mask:    Paritipant certificates common name mask in format of name{number}"
   echo "--group-size: Number of participants signed by generated CA"
fi

#default arguments:
DAYS=1
CA_ORG="Block.one"
CA_CN="test-domain"
ORG_MASK="node{NUMBER}"
CN_MASK="test-domain{NUMBER}"
GROUP_SIZE=4

#overrides default is set
parse-args "${@}"

echo "*************************************************"
echo "         generating CA_cert.pem                  "
echo "*************************************************"

openssl req -newkey rsa:2048 -nodes -keyout CA_key.pem -x509 -days ${DAYS} -out CA_cert.pem -subj "/C=US/ST=VA/L=Blocksburg/O=${CA_ORG}/CN=${CA_CN}"

echo "*************************************************"
openssl x509 -in CA_cert.pem -text -noout

echo "*************************************************"
echo "         generating nodes certificates    "
echo "*************************************************"

#client certificate requests + private keys
for n in $(seq 0 $(($GROUP_SIZE-1)) )
do
   ORG_NAME=$(sed "s/{NUMBER}/$n/" <<< "$ORG_MASK")
   CN_NAME=$(sed "s/{NUMBER}/$n/" <<< "$CN_MASK")
   echo "*************************************************"
   echo "generating certificate for $ORG_NAME / $CN_NAME  "
   echo "*************************************************"
   openssl req -newkey rsa:2048 -nodes -keyout "${ORG_NAME}_key.pem" -out "${ORG_NAME}.csr" -subj "/C=US/ST=VA/L=Blockburg/O=${ORG_NAME}/CN=${CN_NAME}"
   openssl x509 -req -in "${ORG_NAME}.csr" -CA CA_cert.pem -CAkey CA_key.pem -CAcreateserial -out "${ORG_NAME}.crt" -days ${DAYS} -sha256
   echo "*************************************************"
   openssl x509 -in "${ORG_NAME}.crt" -text -noout
   echo ""
done