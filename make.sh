echo Building RunServer
go build
echo Compiling runner and compiler
g++ ./Cjudger/runner.cc -o runner
g++ ./Cjudger/compiler.cc -o compiler
echo Building similarity test
cd sim/sim_2_67
make binaries 
cp sim_c sim_java sim_text ../
make fresh
echo Success
