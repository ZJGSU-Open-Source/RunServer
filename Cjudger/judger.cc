//This file if forked from https://github.com/siqiaochen/oj_client_hustoj/
//it hasn't been tested, so don't use it in your OJ.
//This file is in golang format, don't change.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include "okcalls.h"
#include "judger.h"

const int DEBUG = 0;

static int port_number;
static int max_running;
static int sleep_time;
static int java_time_bonus = 5;
static int java_memory_bonus = 512;
static char java_xms[BUFFER_SIZE];
static char java_xmx[BUFFER_SIZE];
static int use_max_time=0;

//record system call
//标志：是否记录系统系统调用
static char record_call=0;

static char lang_ext[3][8] = { "c", "cc", "java"};

//get_file_size get the specific file size
//获取文件大小
long get_file_size(const char * filename){
    struct stat f_stat;
    if (stat(filename, &f_stat) == -1){
        return 0;
    }
    return (long) f_stat.st_size;
}

//write_log write log to file oj_home/log/client.log
//将log信息写入oj_home/log/client.log文件
void write_log(const char *fmt, ...){
    va_list ap;
    char buffer[4096];
    sprintf(buffer,"%s/log/client.log",oj_home);
    FILE *fp = fopen(buffer, "a+");
    if (fp == NULL){
        fprintf(stderr, "openfile error!\n");
        system("pwd");
    }
    va_start(ap, fmt);

    vsprintf(buffer, fmt, ap);
    fprintf(fp, "%s\n", buffer);
    if (DEBUG)
        printf("%s\n", buffer);
    va_end(ap);
    fclose(fp);
}

//执行命令
int execute_cmd(const char * fmt, ...){
    char cmd[BUFFER_SIZE];

    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system(cmd);
    va_end(ap);
    return ret;
}

const int call_array_size=512;
int call_counter[call_array_size]= {0};
static char LANG_NAME[BUFFER_SIZE];
//init_syscalls_limits
//初始化系统调用限制表
void init_syscalls_limits(int lang){
    int i;
    memset(call_counter, 0, sizeof(call_counter));
    if (DEBUG)
        write_log("init_call_counter:%d", lang);
    if (record_call){   // C & C++
        for (i = 0; i<call_array_size; i++){
            call_counter[i] = 0;
        }
    }else if (lang <= LangCC){    // C & C++
        for (i = 0; LANG_CC[i]; i++){
            call_counter[LANG_CV[i]] = LANG_CC[i];
        }
    }else if (lang == LangJava){     // Java
        for (i = 0; LANG_JC[i]; i++)
            call_counter[LANG_JV[i]] = LANG_JC[i];
    }
}

//isInFile cheeck whether the filename ends with suffix ".in"
//检查文件后缀是否为".in"
int isInFile(const char fname[]){
    int l = strlen(fname);
    if (l <= 3 || strcmp(fname + l - 3, ".in") != 0){
        return 0;
    }else{
        return l - 3;
    }
}

//比较用户输出和标准数据
int compare(const char *file1, const char *file2){
	return OJ_AC;
}

//获取命令输出
FILE * read_cmd_output(const char * fmt, ...){
    char cmd[BUFFER_SIZE];

    FILE *  ret =NULL;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    va_end(ap);
    if(DEBUG){
    	printf("%s\n",cmd);
    }
    ret = popen(cmd,"r");

    return ret;
}

//编译
int compile(int lang){
    int pid;

    const char * CP_C[] = { "gcc", "Main.c", "-o", "Main","-Wall", "-lm",
                            "--static", "-std=c99", "-DONLINE_JUDGE", NULL
                          };
    const char * CP_X[] = { "g++", "Main.cc", "-o", "Main", "-Wall",
                            "-lm", "--static","-std=c++0x", "-DONLINE_JUDGE", NULL
                          };
	//const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m", "Main.java",NULL };

    char javac_buf[4][16];
    char *CP_J[5];
    for(int i=0; i<4; i++) CP_J[i]=javac_buf[i];
    sprintf(CP_J[0],"javac");
    sprintf(CP_J[1],"-J%s",java_xms);
    sprintf(CP_J[2],"-J%s",java_xmx);
    sprintf(CP_J[3],"Main.java");
    CP_J[4]=(char *)NULL;

    pid = fork();
    if (pid == 0){
        struct rlimit LIM;
        LIM.rlim_max = 600;
        LIM.rlim_cur = 600;
        setrlimit(RLIMIT_CPU, &LIM);

        LIM.rlim_max = 900 * STD_MB;
        LIM.rlim_cur = 900 * STD_MB;
        setrlimit(RLIMIT_FSIZE, &LIM);

        LIM.rlim_max =  STD_MB<<11;
        LIM.rlim_cur =  STD_MB<<11;
        setrlimit(RLIMIT_AS, &LIM);
        
        freopen("ce.txt", "w", stdout);//record copmile error
        
        switch (lang){
        case LangC:
            execvp(CP_C[0], (char * const *) CP_C);
            break;
        case LangCC:
            execvp(CP_X[0], (char * const *) CP_X);
            break;
        case LangJava:
            execvp(CP_J[0], (char * const *) CP_J);
            break;
        default:
            printf("nothing to do!\n");
        }
        if (DEBUG){
            printf("compile end!\n");
        }

        exit(0);
    }else{
        int status=0;
        waitpid(pid, &status, 0);
        if (DEBUG){
            printf("status = %d\n", status);
        }
        return status;
    }

}

//获取进程状态
int get_proc_status(int pid, const char * mark){
    FILE * pf;
    char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
    int ret = 0;
    sprintf(fn, "/proc/%d/status", pid);
    pf = fopen(fn, "r");
    int m = strlen(mark);
    while (pf && fgets(buf, BUFFER_SIZE - 1, pf)){
        buf[strlen(buf) - 1] = 0;
        if (strncmp(buf, mark, m) == 0){
            sscanf(buf + m + 1, "%d", &ret);
        }
    }
    if(pf){
        fclose(pf);
    }
    return ret;
}


//准备测试文件
void prepare_files(char * filename, int namelen, char * infile, char * work_dir, char * outfile, char * userfile){
    char  fname[BUFFER_SIZE];
    strncpy(fname, filename, namelen);
    fname[namelen] = 0;
    sprintf(infile, "%s/ProblemData/%s.in", oj_home, fname);
    execute_cmd("cp %s %s/data.in", infile, work_dir);

    sprintf(outfile, "%s/ProblemData/%s.out", oj_home, fname);
    sprintf(userfile, "%s/run/user.out", oj_home);
}

//运行编译后的程序
void run_solution(int & lang, char * work_dir, int & time_lmt, int & usedtime, int & mem_lmt){
    nice(19);
    chdir(work_dir);
    // open the files
    freopen("data.in", "r", stdin);
    freopen("user.out", "w", stdout);
    freopen("error.out", "a+", stderr);
    
    // run me
    if (lang != 3)
        chroot(work_dir);

    // while(setgid(1536)!=0) sleep(1);
    // while(setuid(1536)!=0) sleep(1);
    // while(setresuid(1536, 1536, 1536)!=0) sleep(1);

	// char java_p1[BUFFER_SIZE], java_p2[BUFFER_SIZE];
    // child
    // set the limit
    struct rlimit LIM; // time limit, file limit& memory limit
    // time limit

    LIM.rlim_cur = (time_lmt - usedtime / 1000) + 1;
    LIM.rlim_max = LIM.rlim_cur;
    //if(DEBUG) printf("LIM_CPU=%d",(int)(LIM.rlim_cur));
    setrlimit(RLIMIT_CPU, &LIM);
    alarm(0);
    alarm(time_lmt*10);

    // file limit
    LIM.rlim_max = STD_F_LIM + STD_MB;
    LIM.rlim_cur = STD_F_LIM;
    setrlimit(RLIMIT_FSIZE, &LIM);
    
    // proc limit
    if(lang == LangJava){  //java
        LIM.rlim_cur=LIM.rlim_max = 50;
    }else{
        LIM.rlim_cur=LIM.rlim_max = 1;
    }

    setrlimit(RLIMIT_NPROC, &LIM);

    // set the stack
    LIM.rlim_cur = STD_MB << 6;
    LIM.rlim_max = STD_MB << 6;
    setrlimit(RLIMIT_STACK, &LIM);
    // set the memory
    LIM.rlim_cur = STD_MB *mem_lmt/2*3;
    LIM.rlim_max = STD_MB *mem_lmt*2;
    if(lang != LangJava){
        setrlimit(RLIMIT_AS, &LIM);
    }

    // trace me
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    
    if(lang == LangC || lang ==LangCC){
        execl("./Main", "./Main", (char *)NULL);
    }else if(lang == LangJava){
        // sprintf(java_p1, "-Xms%dM", mem_lmt / 2);
        // sprintf(java_p2, "-Xmx%dM", mem_lmt);
        execl("/usr/bin/java", "/usr/bin/java", java_xms,java_xmx,
              "-Djava.security.manager",
              "-Djava.security.policy=./java.policy", "Main", (char *)NULL);
    }
    //sleep(1);
    exit(0);
}

int fix_java_mis_judge(char *work_dir, int & JudgeFlag, int & topmemory, int mem_lmt){
    int comp_res = OJ_AC;
    if (DEBUG)
        execute_cmd("cat %s/error.out", work_dir);
    comp_res = execute_cmd("grep 'java.lang.OutOfMemoryError'  %s/error.out",
                           work_dir);

    if (!comp_res){
        printf("JVM need more Memory!");
        JudgeFlag = OJ_ML;
        topmemory = mem_lmt * STD_MB;
    }
    comp_res = execute_cmd("grep 'java.lang.OutOfMemoryError'  %s/user.out",
                           work_dir);

    if (!comp_res){
        printf("JVM need more Memory or Threads!");
        JudgeFlag = OJ_ML;
        topmemory = mem_lmt * STD_MB;
    }
    comp_res = execute_cmd("grep 'Could not create'  %s/error.out", work_dir);

    if (!comp_res){
        printf("jvm need more resource,tweak -Xmx(OJ_JAVA_BONUS) Settings");
        JudgeFlag = OJ_RE;
        //topmemory=0;
    }
    return comp_res;
}

//评判用户solution
void judge_solution(int & JudgeFlag, int & usedtime, int time_lmt,
                    char * infile, char * outfile, char * userfile,
                    int lang, char * work_dir, int & topmemory, int mem_lmt){
    //usedtime-=1000;
    int comp_res;
    if (usedtime > time_lmt * 1000){
        JudgeFlag = OJ_TL;
		return;
	}
    if(topmemory > mem_lmt * STD_MB){
		JudgeFlag = OJ_ML; //issues79
		return;
	}
	// compare
	JudgeFlag = compare(outfile, userfile);
	if (JudgeFlag == OJ_WA && DEBUG){
			printf("fail test %s\n", infile);
	}

	//jvm popup messages, if don't consider them will get miss-WrongAnswer
    if (lang == 3 && JudgeFlag != OJ_AC){
        comp_res = fix_java_mis_judge(work_dir, JudgeFlag, topmemory, mem_lmt);
    }
}

int get_page_fault_mem(struct rusage & ruse, pid_t & pidApp){
    //java use pagefault
    int m_vmpeak, m_vmdata, m_minflt;
    m_minflt = ruse.ru_minflt * getpagesize();
    if (0 && DEBUG){
        m_vmpeak = get_proc_status(pidApp, "VmPeak:");
        m_vmdata = get_proc_status(pidApp, "VmData:");
        printf("VmPeak:%d KB VmData:%d KB minflt:%d KB\n", m_vmpeak, m_vmdata,
               m_minflt >> 10);
    }
    return m_minflt;
}

//输出运行错误
void print_runtimeerror(char * err){
    FILE *ferr=fopen("error.out","a+");
    fprintf(ferr,"Runtime Error:%s\n",err);
    fclose(ferr);
}

//观察用户程序运行
void watch_solution(pid_t pidApp, char *infile, int &JudgeFlag,
                    char *userfile, char *outfile, int lang,
                    int &topmemory, int mem_lmt, int &usedtime, int time_lmt, char *work_dir){
    // parent
    int tempmemory;

    if (DEBUG){
        printf("pid=%d judging %s\n", pidApp, infile);
    }

    int status, sig, exitcode;
    struct user_regs_struct reg;
    struct rusage ruse;
    int sub = 0;
    while (1){
        // check the usage
        wait4(pidApp, &status, 0, &ruse);

		//jvm gc ask VM before need,so used kernel page fault times and page size
        if (lang == LangJava){
            tempmemory = get_page_fault_mem(ruse, pidApp);
        }else{    //other use VmPeak
            tempmemory = get_proc_status(pidApp, "VmPeak:") << 10;
        }
        if (tempmemory > topmemory){
            topmemory = tempmemory;
        }
        if (topmemory > mem_lmt * STD_MB){
            if (DEBUG){
                printf("out of memory %d\n", topmemory);
			}
			JudgeFlag = OJ_ML;
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }

        if (WIFEXITED(status)){
            break;
        }
        if (get_file_size("error.out")){
            JudgeFlag = OJ_RE;
            //addreinfo(solution_id);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }

        if (get_file_size(userfile) > get_file_size(outfile) * 2+1024){
            JudgeFlag = OJ_OL;
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }

        exitcode = WEXITSTATUS(status);
        /*exitcode == 5 waiting for next CPU allocation
         *  */
        if ((lang == 3 && exitcode == 17) || exitcode == 0x05 || exitcode == 0){
            //go on and on
            ;
        }else{
            if (DEBUG){
                printf("status>>8=%d\n", exitcode);
            }
            //psignal(exitcode, NULL);
            switch (exitcode){
				case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    JudgeFlag = OJ_TL;
                    break;
                case SIGXFSZ:
                    JudgeFlag = OJ_OL;
                    break;
                default:
                    JudgeFlag = OJ_RE;
			}
            print_runtimeerror(strsignal(exitcode));
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        if (WIFSIGNALED(status)){
            /*  WIFSIGNALED: if the process is terminated by signal
             *
             *  psignal(int sig, char *s)，like perror(char *s)，print out s, with error msg from system of sig
            * sig = 5 means Trace/breakpoint trap
            * sig = 11 means Segmentation fault
            * sig = 25 means File size limit exceeded
            */
            sig = WTERMSIG(status);

            if (DEBUG){
                printf("WTERMSIG=%d\n", sig);
                psignal(sig, NULL);
            }
			switch (sig){
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    JudgeFlag = OJ_TL;
                    break;
                case SIGXFSZ:
                    JudgeFlag = OJ_OL;
                    break;
                default:
                    JudgeFlag = OJ_RE;
			}
            print_runtimeerror(strsignal(sig));
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);//new add by sake
            break;
        }
        /*     comment from http://www.felix021.com/blog/read.php?1662

        WIFSTOPPED: return true if the process is paused or stopped while ptrace is watching on it
        WSTOPSIG: get the signal if it was stopped by signal
         */

        // check the system calls
        ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);

        if (!record_call&&call_counter[reg.REG_SYSCALL] == 0) {   //do not limit JVM syscall for using different JVM
            JudgeFlag = OJ_RE;
            char error[BUFFER_SIZE];
            sprintf(error,"[ERROR] A Not allowed system call! callid:%ld\n",reg.REG_SYSCALL);
            write_log(error);
            print_runtimeerror(error);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
        }else if(record_call){
            call_counter[reg.REG_SYSCALL]=1;
        }else{
            if (sub == 1 && call_counter[reg.REG_SYSCALL] > 0){
                call_counter[reg.REG_SYSCALL]--;
            }
        }
        sub = 1 - sub;

        ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
    }
    usedtime += (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
    usedtime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);
}


//清空运行环境
void clean_workdir(char * work_dir ){
    if (DEBUG){
        execute_cmd("mv %s/* %s/log/", work_dir, oj_home);
    }else{
        execute_cmd("rm -Rf %s/*", work_dir);
    }

}

//参数初始化
void init_parameters(int argc, char ** argv, int & lang, int & time_lmt, int & mem_lmt){
    if (argc < 3){
        fprintf(stderr, "Usage:%s [language] [time limit] [memory limit].\n", argv[0]);
        exit(1);
    }

    chdir(oj_home); // change the dir// init our work

    lang = atoi(argv[1]);
    time_lmt = atoi(argv[2]);
    mem_lmt = atoi(argv[3]);
}

//计算in文件的个数
int count_in_files(char * dirpath){
    const char  * cmd="ls -l %s/*.in|wc -l";
    int ret=0;
    FILE * fjobs=read_cmd_output(cmd,dirpath);
    fscanf(fjobs,"%d",&ret);
    pclose(fjobs);

    return ret;
}

//输出用户程序的所用系统调用及调用次数
void print_call_array(){
    printf("int LANG_%sV[256]={",LANG_NAME);
    int i=0;
    for (i = 0; i<call_array_size; i++){
        if(call_counter[i]){
            printf("%d,",i);
        }
    }
    printf("0};\n");

    printf("int LANG_%sC[256]={",LANG_NAME);
    for (i = 0; i<call_array_size; i++){
        if(call_counter[i]){
            printf("HOJ_MAX_LIMIT,");
        }
    }
    printf("0};\n");
}

//judger 程序入口
int main(int argc, char** argv){

    char work_dir[BUFFER_SIZE];
    int time_lmt = 5, mem_lmt = 128, lang;//单位是什么?

    init_parameters(argc, argv, lang, time_lmt, mem_lmt);
	
	//TODO get solution.

    //set work directory to start running & judging
    sprintf(work_dir, "%s/run/", oj_home);
    chdir(work_dir);
    if (!DEBUG){
        clean_workdir(work_dir);
    }

    //java is lucky
    if (lang == LangJava){
        // the limit for java
        time_lmt = time_lmt + java_time_bonus;
        mem_lmt = mem_lmt + java_memory_bonus;
        // copy java.policy
        execute_cmd( "cp %s/etc/java0.policy %sjava.policy", oj_home, work_dir);
    }

    //never bigger than judged set value;
    if (time_lmt > 30 || time_lmt < 1){
        time_lmt = 30;
    }
    if (mem_lmt > 1024 || mem_lmt < 1){
        mem_lmt = 1024;
    }

    if (DEBUG){
        printf("time: %d mem: %d\n", time_lmt, mem_lmt);
    }

    // begin compile
    // set the result to compiling
    int Compile_OK;
    Compile_OK = compile(lang);
    if (Compile_OK != 0){
        //update_solution(OJ_CE, 0, 0, 0,0, 0.0);
        if (!DEBUG){
            clean_workdir(work_dir);
        }else{
            write_log("compile error");
        }
        exit(0);
    }else{
        //update_solution(OJ_RI, 0, 0, 0,0, 0.0);
    }
    // end compile

    // begin run
    char fullpath[BUFFER_SIZE];
    char infile[BUFFER_SIZE];
    char outfile[BUFFER_SIZE];
    char userfile[BUFFER_SIZE];
    sprintf(fullpath, "%s/ProblemData/", oj_home); // the fullpath of data dir

    // open DIRs
    DIR *dp;
    dirent *dirp;

    if ((dp = opendir(fullpath)) == NULL){
        write_log("No such dir:%s!\n", fullpath);
        exit(-1);
    }

    int JudgeFlag = OJ_AC;
    int namelen;
    int usedtime = 0, topmemory = 0;


    // read files and run

    for (; ( JudgeFlag == OJ_AC )&& (dirp = readdir(dp)) != NULL;){
	// for(; ( JudgeFlag == OJ_AC )&& (dirp = readdir(dp)) != NULL;{
        namelen = isInFile(dirp->d_name); // check whether the file is *.in or not
        if (namelen == 0){
            continue;
        }

        prepare_files(dirp->d_name, namelen, infile, work_dir, outfile, userfile);
        init_syscalls_limits(lang);

        pid_t pidApp = fork();
        if (pidApp == 0){
            run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
			exit(0);
        }else{
			watch_solution(pidApp, infile, JudgeFlag, userfile, outfile, lang, topmemory, mem_lmt, usedtime, time_lmt, work_dir);

			if(JudgeFlag == OJ_AC){
				judge_solution(JudgeFlag, usedtime, time_lmt, infile, outfile, userfile, lang, work_dir, topmemory, mem_lmt);
			}
        }
    }

    if(JudgeFlag == OJ_RE && DEBUG){
        printf("add RE info ..... \n");
    }

	if(JudgeFlag == OJ_TL){
        usedtime=time_lmt*1000;
    }
  
    if(JudgeFlag == OJ_WA && DEBUG) {
        printf("add diff info of ..... \n");
    }

    clean_workdir(work_dir);

    if (DEBUG){
        write_log("result = %d usedtime = %d topmemory = %d", JudgeFlag,usedtime,topmemory);
    }

    if(record_call){
        print_call_array();
    }

    return 0;
}
