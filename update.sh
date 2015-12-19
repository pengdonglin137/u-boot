#!/bin/bash

remotes="`git remote show`";
flags=0

for i in $remotes
do
	if [ "$i" = "latest" ]
	then
		flags=1
	fi
done

if [ $flags -eq 0 ]
then
	git remote add latest git://git.denx.de/u-boot.git
fi

git fetch latest
git checkout master
git merge remotes/latest/master
git push -u origin master
