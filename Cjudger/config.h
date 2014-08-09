#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 512

const int JudgeNA  = 0;  //None
const int JudgePD  = 1; //Pending
const int JudgeRJ  = 2;  //Running & judgingconst
const int JudgeAC  = 3;  //Accepted
const int JudgeCE  = 4; //Compile Error
const int JudgeRE  = 5;  //Runtime Error
const int JudgeWA  = 6;  //Wrong Answer
const int JudgeTLE = 7;  //Time Limit Exceeded
const int JudgeMLE = 8;  //Memory Limit Exceeded
const int JudgeOLE = 9;  //Output Limit Exceeded
const int JudgeCP  = 10; //wait to Compare Output


const int LangN 	= 0;
const int LangC 	= 1;
const int LangCC	= 2;
const int LangJava 	= 3;

const char *oj_home = "/home/sake/Projects/GO/src";

#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi

#endif
