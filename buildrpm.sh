#!/usr/bin/env bash

# verbose (x), fail on non-zero exit (e)
set -xe

NAME=ssss
SPECFILE="${BASH_SOURCE%/*}/${NAME}.spec"

COMMIT=$(git rev-parse --short --verify HEAD)
TAG=${1:-${COMMIT}}

RPM_BUILD_TOPDIR=$(rpmbuild --eval '%{_topdir}')
TARBALL_FILENAME="${NAME}-${TAG}.tar.gz"
TARBALL_PATH="${RPM_BUILD_TOPDIR}/SOURCES/${TARBALL_FILENAME}"

git archive --prefix "${NAME}-${TAG}/" --output "${TARBALL_PATH}" ${TAG}
 
rpmbuild --define "_version ${TAG}" -bb "${SPECFILE}"
cp ${RPM_BUILD_TOPDIR}/RPMS/x86_64/${NAME}-${TAG}-*.rpm . 
