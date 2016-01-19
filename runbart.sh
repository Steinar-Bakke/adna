#!/bin/bash
if [ "$1" == "" ] || [ "$2" == "" ] || [ "$3" == "" ]; then
	echo "Usage: $0 <num_processes> <fastq1> <fastq2>"
	echo
	exit 1
fi
LD_LIBRARY_PATH=/usr/local/gcc-5.3.0/lib64/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH
mkdir -p results
rm -rf ./results/*
/usr/local/gcc-5.3.0/openmpi/bin/mpirun -np $1 ./bin/adna $2 $3

id=$1
id=${id:0:-5}
r1P=$id+="1Pass.fastq"
r2p=$id+="2Pass.fastq"
r1F=$id+="1Fail.fastq"
r2F=$id+="2Fail.fastq"
sRP=$id+="singlePass.fastq"
sRF=$id+="singleFail.fastq"


for i in `ls | grep -E "*read1Pass*"` ; do
	cat $i >> $r1P
done

for i in `ls | grep -E "*read2Pass*"` ; do
	cat $i >> $r2P
done

for i in `ls | grep -E "*read1Fail*"` ; do
	cat $i >> $r1F
done

for i in `ls | grep -E "*read2Fail*"` ; do
	cat $i >> $r2F
done

for i in `ls | grep -E "*singleReadPass*"` ; do
	cat $i >> $sRP
done

for i in `ls | grep -E "*singleReadFail*"` ; do
	cat $i >> $sRF
done
