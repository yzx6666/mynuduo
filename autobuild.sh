#! /bin/bash

set -e

# ���û��buildĿ¼��������Ŀ¼
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make

# �ص���Ŀ��Ŀ¼
cd ..
 
# ��ͷ�ļ������� /usr/include/mymuduo so�⿽���� /usr/lib
if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

# ˢ�¶�̬�⻺��
ldconfig