#!/usr/bin/env bash
export KEYNAME=opkit2
export EMAIL=devops@operator.tc
export KEY_DIR=`pwd`/keys

function create_key {
  rm -rf $KEY_DIR
  mkdir $KEY_DIR

cat > $KEYNAME.batch <<EOF
  %no-protection
  %echo Generating a standard key
  Key-Type: RSA
  Key-Length: 4096
  Subkey-Length: 4096
  Name-Real: ${KEYNAME}
  Name-Email: ${EMAIL}
  Expire-Date: 0
  %commit
  %echo done
EOF

  gpg --batch --gen-key --homedir $KEY_DIR $KEYNAME.batch
  gpg --list-secret-keys --homedir $KEY_DIR --no-default-keyring --secret-keyring ${KEYNAME}.key --keyring ${KEYNAME}.pub
}

function get_key {
  gpg get ${KEYNAME}.key
  #gpg --output keys/socketsupply.gpg --armor --export $KEYID
  #apt-ftparchive --arch amd64 packages amd64 > Packages
  #gzip -k -f Packages
}

create_key
