cd testnet_np
rm -rf blocks blockchain std*
cd ..
../programs/eosd/eosd --data-dir testnet_np --replay-blockchain > testnet_np/stdout.txt 2> testnet_np/stderr.txt &
echo Launched eosd.
echo See testnet_np/stderr.txt for eosd output.
echo Synching requires at least 8 minutes, depending on network conditions.
