#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset
# set -o xtrace

SRC=/buildroot/output/target/
DST=/rootfs/
RSYNC_IGNORE=/workspace/rsyncignore

# Copy all non-existing files from SRC to DST
rsync -rlpgoD --itemize-changes \
  --ignore-existing \
  --exclude=THIS_IS_NOT_YOUR_ROOT_FILESYSTEM \
  $SRC $DST

RSYNC_OPT=""
if [[ -f $RSYNC_IGNORE ]]; then
    RSYNC_OPT="--exclude-from=$RSYNC_IGNORE"
    echo "Using rsyncignore"
fi

# Update all existing files in DST from SRC
rsync -crlpgoD --itemize-changes \
  --exclude=/etc/ssh/** \
  --exclude=/etc/hostname \
  --exclude=/etc/group \
  --exclude=/etc/passwd \
  --exclude=/etc/shadow \
  --exclude=/etc/fstab \
  --exclude=THIS_IS_NOT_YOUR_ROOT_FILESYSTEM \
  $RSYNC_OPT $SRC $DST