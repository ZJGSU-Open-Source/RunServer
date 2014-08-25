#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 512

const int JudgePD  = 0; //Pending
const int JudgeRJ  = 1; //Running & judging
const int JudgeCE  = 2; //Compile Error
const int JudgeAC  = 3; //Accepted
const int JudgeRE  = 4; //Runtime Error
const int JudgeWA  = 5; //Wrong Answer
const int JudgeTLE = 6; //Time Limit Exceeded
const int JudgeMLE = 7; //Memory Limit Exceeded
const int JudgeOLE = 8; //Output Limit Exceeded
const int JudgePE  = 9; //Presentation Error
const int JudgeNA  = 10; //None

const int LangN 	= 0;
const int LangC 	= 1;
const int LangCC	= 2;
const int LangJava 	= 3;

const char *oj_home = "/home/jinwei/Go/src";

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
