patches=$(find $(pwd) -type f -name '*.patch')
#echo $1
echo $patches
for patch in $patches; do
	patch -p0 < $patch
done
