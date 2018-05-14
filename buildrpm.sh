#!/usr/bin/env bash

# verbose (x), fail on non-zero exit (e)
set -xe

NAME=ssss
SPECFILE="${BASH_SOURCE%/*}/${NAME}.spec"

COMMIT=$(git rev-parse --short --verify HEAD)

# if a version tag is passed in, use it instead of short sha1
VERSION=${1:-${COMMIT}}

RPM_BUILD_TOPDIR=${PWD}/rpmbuild
mkdir -p ${RPM_BUILD_TOPDIR}/SOURCES

TARBALL_FILENAME="${NAME}-${VERSION}.tar.gz"
TARBALL_PATH="${RPM_BUILD_TOPDIR}/SOURCES/${TARBALL_FILENAME}"

git archive --prefix "${NAME}-${VERSION}/" --output "${TARBALL_PATH}" HEAD
 
rpmbuild --define "_version ${VERSION}" --define "_topdir ${RPM_BUILD_TOPDIR}" -bb "${SPECFILE}"
cp ${RPM_BUILD_TOPDIR}/RPMS/x86_64/${NAME}-${VERSION}-*.rpm . 
