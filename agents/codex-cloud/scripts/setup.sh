#!/bin/sh

add-apt-repository -y universe
apt-get update -y

apt-get install -y --no-install-recommends \
	ninja-build \
	doxygen \
	graphviz \
	libatomic1 \
	libicu-dev \
	libncurses6 \
	libtomcrypt1 \
	libtommath1

FIREBIRD_PACKAGE=Firebird-5.0.3.1683-0-linux-x64
FIREBIRD_RELEASE_URL=https://github.com/FirebirdSQL/firebird/releases/download/v5.0.3/$FIREBIRD_PACKAGE.tar.gz

mkdir /tmp/firebird-installer
curl -fSL "$FIREBIRD_RELEASE_URL" -o /tmp/firebird-installer/$FIREBIRD_PACKAGE.tar.gz
tar xzf /tmp/firebird-installer/$FIREBIRD_PACKAGE.tar.gz -C /tmp
tar xzf /tmp/firebird-installer/$FIREBIRD_PACKAGE/buildroot.tar.gz -C
rm -rf /tmp/firebird-installer

cat <<'EOF' >> ~/.bashrc
export FIREBIRD=/opt/firebird
export LD_LIBRARY_PATH=$FIREBIRD/lib:$LD_LIBRARY_PATH
export PATH=$FIREBIRD/bin:$PATH
EOF

git submodule update --init --recursive

./gen-debug.sh
