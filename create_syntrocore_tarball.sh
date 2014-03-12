#!/bin/bash

if [ -f Makefile ]; then
        make clean
fi

rm -f *.tgz *.bz2 Makefile *.log* *.ini

subdirs=( SyntroLib SyntroGUI SyntroControlLib SyntroControl SyntroAlert SyntroLog SyntroDB )

for subdir in ${subdirs[@]}; do
        if [ -d $subdir ]; then
                cd $subdir
                rm -f Makefile* *.so*
                rm -rf release debug Output GeneratedFiles
                cd ..
        fi
done

tar -cjf SyntroCore.tar.bz2  --exclude='.git' *


echo "Created: SyntroCore.tar.bz2"
echo -n "Size: "
ls -l SyntroCore.tar.bz2 | awk '{ print $5 }'

echo -n "md5sum: "
md5sum SyntroCore.tar.bz2 | awk '{ print $1 }'

echo -n "sha256sum: "
sha256sum SyntroCore.tar.bz2 | awk '{ print $1 }'

