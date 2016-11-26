
tags=$(ps $1)
ok=no

for tag in $tags
do
	if [ "$ok" == "yes" ]
	then
		ok=no
		echo kill $tag
		kill $tag
		exit
	fi

	if [ "$tag" == "root" ]
	then
		ok=yes
	fi
done
