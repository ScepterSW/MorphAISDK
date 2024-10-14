#!/bin/bash

set -e

. ./build.conf

SUPPORT_MODEL_TYPES=("yolov5" "yolov8" "vzense_box")

support_model=0
for type in "${SUPPORT_MODEL_TYPES[@]}"; do
  if [[ "${MODEL_TYPE}" = "${type}" ]]; then 
    support_model=1
    break
  fi
done

if ((${support_model} != 1)); then
  echo "${MODEL_TYPE}" is invalid, current support model types:"${SUPPORT_MODEL_TYPES[@]}"
  exit -1
fi

if [[ -z ${CROSS_COMPILER_PATH} ]];then
  echo "Please set CROSS_COMPILER_PATH"
  echo "such as CROSS_COMPILER_PATH=/Morph/1_cross_compiler/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf"
  exit -1
else
  # for rv1126 armhf
  GCC_COMPILER=${CROSS_COMPILER_PATH}/bin/arm-linux-gnueabihf
fi

if [ -z ${MODEL_TYPE} ]; then
  echo "Please set MODEL_TYPE"
  echo "such as MODEL_TYPE=yolov5/yolov8/vzense_box"
  exit -1
fi

# Debug / Release
if [[ -z ${BUILD_TYPE} ]];then
    BUILD_TYPE=Release
fi

ROOT_PWD=$( cd "$( dirname $0 )" && cd -P "$( dirname "$SOURCE" )" && pwd )
INSTALL_DIR=${ROOT_PWD}/install
BUILD_DIR=${ROOT_PWD}/build

if [ ! -d "${BUILD_DIR}" ];then
  mkdir ${BUILD_DIR}
fi

if [[ -d "${INSTALL_DIR}" ]]; then
  rm -rf ${INSTALL_DIR}
fi

echo "==================================="
echo "GCC_COMPILER=${GCC_COMPILER}"
echo "MODEL_TYPE=${MODEL_TYPE}"
echo "BUILD_TYPE=${BUILD_TYPE}"
echo "INSTALL_DIR=${INSTALL_DIR}"
echo "BUILD_DIR=${BUILD_DIR}"
echo "==================================="

cd  ${BUILD_DIR}
pwd
cmake \
    -DMODEL_TYPE=${MODEL_TYPE} \
    -DCMAKE_C_COMPILER=${GCC_COMPILER}-gcc \
    -DCMAKE_CXX_COMPILER=${GCC_COMPILER}-g++ \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} ..

if [ $BUILD_ALGLIB -eq 1 ]; then
  cd ${BUILD_DIR}/algLib
  make -j8
  make install
  cd ${BUILD_DIR}
fi

if [ $BUILD_LIBTEST -eq 1 ]; then
  cd ${BUILD_DIR}/algLib_test_example
  make -j8
  make install
  cd ${BUILD_DIR}
fi

MODEL_DIRECTORY="${INSTALL_DIR}/model"
if [ ! -d "${MODEL_DIRECTORY}" ];then
  mkdir "${MODEL_DIRECTORY}"
fi

cp ${ROOT_PWD}/${MODEL_FILE_PATH} "${MODEL_DIRECTORY}/test.rknn"
cp -r ${ROOT_PWD}/${TEST_IMAGES_PATH} ${INSTALL_DIR}