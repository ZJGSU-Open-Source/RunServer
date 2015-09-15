#RunServer

Judger and Dispatcher for ZJGSU New OJ

##Docker

```bash
docker build -t judger .
docker run -e ["DATA_PATH=your_data_path","RUN_PATH=your_run_path","LOG_PATH=your_log_path"] -v your_data_path:your_data_path -d -p 80:8888 oj
```

##Install Dependences
```bash
sudo apt-get install -y build-essential flex openjdk-7-jdk 
```

##Build
```bash
./make.sh
```

##Run
```bash
RunServer &
```
