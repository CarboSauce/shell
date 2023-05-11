#!./build/debug/shell.out
echo -e "bbb\naaa\nccc\naaa" >| /tmp/test.txt
cat /tmp/test.txt | sort | uniq
