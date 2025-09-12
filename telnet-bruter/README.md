# Telnet Bruter

This tool is used for creating a telnet combo list by
using an list of ip adress. It will try a dozen of default
password and send you the result in stdout and in a file.

### Installation
Install the app on the server
```sh

1
apt update
apt install git -y

1.2
apt update
apt install -y telnet 

2
apt update
apt install build-essential -y

3
apt update
apt install zmap -y

4
zmap --version
gcc --version
make --version
git --version
telnet --version
nc --version
5
 git clone https://github.com/SystemVll/telnet-bruter.git
 cd  ./telnet-bruter/
 gcc ./*.c -o telnet-bruter -pthread -std=c99 -fcommon
 zmap -p 23  | ./telnet-bruter 200
```

<!-- hostname -I что бы узнать айпи --> 192.168.1.244 
![image](https://user-images.githubusercontent.com/69421356/192002873-c8f5fd0d-9866-43dc-a18a-59b9ddf051f3.png)
