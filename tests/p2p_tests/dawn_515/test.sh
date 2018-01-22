#!/bin/bash

###############################################################
# -p <producing nodes count>
# -n <total nodes>
# -s <topology>
# -d <delay between nodes startup>
###############################################################

pnodes=1
total_nodes=3
topo=star
delay=1

rm -rf tn_data_*
sed '/^#!/,/^__DATA__$/D' "$0" | uudecode -p -m | tar -zxf -
programs/launcher/launcher -p $pnodes -n $total_nodes --nogen -d $delay

sleep 1
res=$(grep "reason = duplicate" tn_data_0*/stderr.txt | wc -l)

if [ $res -ne 0 ]; then
    echo FAILURE: got a \"duplicate\" message
else
    echo "SUCCESS"
fi

programs/launcher/launcher -k 15
exit $res

__DATA__
begin-base64 644 configs.tgz
H4sIAIt8ZVoAA+2ZUVPbOBDHefan8PgZg21iO8nMPdwdbW961x40Hdpeh8kothILjORIcnKh0+9+
a4ccDlGAh0zSlP0xg5P/yitb611rQWkyYnx0rHk/JZr0Pe/4YNN4QByG82MU1Udgcaw/+yfBiR/E
ERwPvOpLdGCHG78SA6XSRNr2QZGx/LFx04zSRwfsJ2o1/rkYVdLRlRJ8I3NUAY5arXXx9+Mgvot/
FMcQeM8PIzDb3kZmf4IXHv9vlm07jCd5mVLldO2vl4eVQoqC8pTKWqrGVDic3FAQHKXBJJ3Dha5n
Ra0ngiuR03sDkaPKw8KBXZ0qKbm5c9IHL+LeD5hzOqF5PxG5WJ66aa5OTumgHDVOrK31aZV1JCnl
TsP4/XCdoymRfK2fgRTTZ/p5eCfLjiRNl9z8//nSWjheLBnlZJDD8K6tZUmt5VmbERCl3kAEml5w
/desP/yep0VVGtclRUqHpMwbMVmzWA9muI9UmjLNJkzPwDIkuWqYmtnYjGSdhlbzVuprtb4/O/8N
9R+eoSEbHTHONlRjnqj/gRcED97/YYD1fzuMKKeKKbd62du/2EfHd0L99rcGuUiuXXjq3ZRJMNff
lQUFJBU8n4HiWQoeTXeaQdVx52ZQ69RRGYG8c4cMLI3Tk4wwvmRU7JaCte13AivTunAVlRMqXcgI
SVXlD56MIw9+/G4bsIqgcHOmNOUuTF4IxnV1KfUQr9tpx1E9ZMUNTE/yTCg9H0PyXEzhIuCB5zTR
rF4Bwmdwf+OSVZdXEKlZwgpyZ6zvq5BsQjR1r2m1AF+dV3/3oncfZr9enY/bZRpnF++/JEN+cfZ2
nFwUKnkf9kT7t0z/Ub75Mn71MXxz+in8/cI5dMI/z6fybDBN/4rOss/l1b+fTuLXvd75P/5bNlW9
j61kfH57Smef9Vncub2evD49cS5hdpGWCdxWVXdgfkhTTYzqwKgmRjU1qtSoDo3qyKhmRpUZ1Suj
em1Uc6N6Y1S5URVGtTCqY6MqjaoyqtqollaRl1B+4SsVioludzGqPzes2Ovk6ZOCrRtAkkSUXPcz
SA8hZ88d1vC464qEbJOV97+/4/4/bNX9f+sE+/9tYIj/j9H/+7j/2wbY/+9B/4n9/27X/yX1//7W
+38vCoMH7/8w8FtY/7fBHvb/naf7//gZ/X+8vv+vTi7oY386+Gn6spX8D3a8/4993P9vEUP8f4z9
f4D1fxvg/n8P9p+4/9/t+r+k/X+w0/2/3wq9+f//Yqz/22D/9v8d7+n9fxv3/wiCIAiCIAiCIAiC
IAiCIAiCIAiCIAiCIAiCIAiC/LT8B31T5+MAUAAA
====
