# Usage: 
# echo '
# 0# 0x0000000001349890 in ./nodeos
# 1# 0x0000000001346BD5 in ./nodeos
# 2# 0x0000000000845744 in ./nodeos
# 3# 0x0000000000885AF1 in ./nodeos
# 4# 0x00000000006BB762 in ./nodeos
# 5# 0x00000000006BB701 in ./nodeos
# 6# 0x00000000006910D1 in ./nodeos
# 7# 0x00000000004A48E3 in ./nodeos
# 8# 0x0000000000490A18 in ./nodeos
# 9# 0x000000000048AF83 in ./nodeos
# ' | awk -f strace_decode.awk
{
	if (match($2, /0x.*/)) { 
		"addr2line -Cfipe " $4 " " $2 | getline result;
		print $1 " " result;
	} else {
		print $0;
        }
}
