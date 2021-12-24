#!/bin/bash

#check environment
if [ ${#DEVICE_NAME} -gt 0 ]
then
	echo " ## Device Name : "$DEVICE_NAME" ##"
else
	echo " #### do \"source env.sh\" at project top directory ####"

	exit 0
fi

OPTIONFILE="optionfile"

# check $2, $3 parameter

if [ "$1" = "s" ]
then
	echo "$2" > $OPTIONFILE
	echo "$3" >> $OPTIONFILE
fi

# check compile option file exist
if [ -e $OPTIONFILE ]
then
	echo "found file : "$OPTIONFILE
else
# make null file
	echo "" > $OPTIONFILE
fi

# declare variable
declare -a ALIST
declare -i count=0

declare -i buildnumber=0
VERSIONINFO="100"

# file open
exec 10<$OPTIONFILE

# read file data
while read LINE <&10; do

ALIST[$count]=$LINE
((count+=1))

done

# file close
exec 10>&-

# store number of read data
ELEMENTS=${#ALIST[@]}

# check number of read data
if (( ELEMENTS > 1))
then
	echo "found S/W version & build number"

VERSIONINFO=${ALIST[0]}
buildnumber=${ALIST[1]}

((buildnumber+=1))

((buildnumber = buildnumber % 1000))

echo "Current S/W version is : "$VERSIONINFO
echo "Current build number is : "$buildnumber

else
echo "no S/W version"
echo "no Build number"
fi

#########################################################################
#########################################################################

TARGETFILE=$DEVICE_NAME"_app"

# toolchain lib
LD_LIBRARY_PATH=/usr/local/arm/4.3.1-eabi-armv6/gmp/lib:/usr/local/arm/4.3.1-eabiarmv6/mpfr/lib
# iconv lib
#ICONV_LIBRARY_PATH=$PROJECT_DIRECTORY"/util/lib/libiconv-1.14/_install/lib"
ICONV_LIBRARY_PATH=$PROJECT_DIRECTORY"/iwt_app/lib"

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib

make clean

# check compile option file exist
if [ -e .depend ]
then
	echo "found file : .depend"
else
	echo " == make dependency =="
	make dep
fi


make -j3 DFLAGS="-DBUILD_NUMBER=$buildnumber -DSW_VERSION=$VERSIONINFO" #컴파일 속도 개선
# make DFLAGS="-DBUILD_NUMBER=$buildnumber -DSW_VERSION=$VERSIONINFO"

if [ -e $TARGETFILE ]
	then
	echo "#######----------- build Release OK ------------------"
	echo


BUILD_NUM=`printf "%03d" $buildnumber`

APPFILE=""$TARGETFILE"_"$VERSIONINFO"_"$BUILD_NUM""
	mv -f $TARGETFILE $APPFILE

else
	echo "######------------ build Release FAIL!! ----------------"
	echo
	exit 0
fi


# update option file
echo $VERSIONINFO > $OPTIONFILE
echo $buildnumber >> $OPTIONFILE

echo "#######----------- build & copy done ----------------------"
echo




