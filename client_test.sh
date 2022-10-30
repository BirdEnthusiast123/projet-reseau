#!/bin/bash

echo "Testing client.c with the mock : server_mock.c ..."

mkfifo fifo_test
if [ $? != 0 ]; then
    echo "If you are using wsl and the error message was :"
    echo "Operation not permitted"
    echo "You will need to enter these commands :"
    echo "go_back_path=\$(pwd)"
    echo "cd ~ && sudo umount /mnt/c"
    echo "sudo mount -t drvfs C:\\ /mnt/c -o metadata"
    echo "cd $(wslpath C:\\) && cd $(wslpath $(cmd.exe /c "echo %USERPROFILE%") | tr -d '\r')"
    echo 'cd $go_back_path'
    exit 1
fi

make mock_server
./mock_server 5555 > res_server.txt 2> err_server.txt &

make client
if [ $? != 0 ]; then
    echo "Failed to compile client.c, exiting..."
    exit 1
else
    echo "Compilation successful"
fi

make client
if [ $? != 0 ]; then
    echo "Failed to compile client.c, exiting..."
    exit 1
else
    echo "Compilation successful"
fi

echo " "
echo "Testing 2 players on single keyboard"
cat > fifo_test &
./client 127.0.0.1 5555 2 < fifo_test > /dev/null 2> err_client.txt &
echo "Connection between processes : ok"

echo "z" > fifo_test
echo "q" > fifo_test
echo "s" > fifo_test
echo "d" > fifo_test
echo " " > fifo_test
echo "i" > fifo_test
echo "j" > fifo_test
echo "k" > fifo_test
echo "l" > fifo_test
echo "m" > fifo_test

echo " "
echo "Killing processes and retrieving outputs.."
pids=$(pgrep client)
pkill pids
pids=$(pgrep mock_server)
pkill pids
pids=$(pgrep cat)
pkill pids

echo " "
echo "Expected result : UP LEFT DOWN RIGHT WALL in this order for both players"
cat res_server.txt
echo "If inputs are correct, inputs : ok"

echo " "
echo "errors :"
echo "stderr client:"
cat err_client.txt
echo "stderr server:"
cat err_server.txt

echo " "
echo "Removing temporary files"
rm res_server.txt err_client.txt err_server.txt mock_server client fifo_test
echo "Done"
exit 0