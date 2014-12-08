echo Building RunServer
go build
echo Compiling runner and compiler
g++ ./Cjudger/runner.cc -o runner
g++ ./Cjudger/compiler.cc -o compiler
mv runner compiler RunServer ../GoOnlineJudge
echo Building similarity test
cd sim/sim_2_70
make binaries 
cp sim_c sim_java sim_text ../
make fresh
echo Success, please go to ../GoOnlineJudge/ and run ./RunServer
