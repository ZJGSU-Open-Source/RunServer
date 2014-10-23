#RunServer

Judger and Dispatcher for ZJGSU New OJ

##Install Dependences
```bash
sudo apt-get install flex
```

##Build
```bash
vim ./Cjudger/config.h
```

Set variable `oj_home` equals to $GOPATH/src, make sure using absolute path to replace $GOPATH

```bash
./make.sh
```

##Run
```bash
cd ../GoOnlineJudge
./RunServer
```
