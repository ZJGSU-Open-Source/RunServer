RunServer: main.go ./Cjudger/runner.cc ./Cjudger/compiler.cc
	go build
	g++ ./Cjudger/runner.cc -o runner
	g++ ./Cjudger/compiler.cc -o compiler
	mv RunServer runner compiler ../GoOnlineJudge
	bash < ./make.sh
