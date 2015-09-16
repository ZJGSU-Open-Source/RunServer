//
// File:   runner.cc
// Author: sempr
// refacted by sakeven
/*
 * Copyright 2008 sempr <iamsempr@gmail.com>
 *
 * Refacted and modified by sakeven<jc5930@sina.cn>
 * Bug report email jc5930@sina.cn
 *
 *
 * This file is part of RunServer.
 *
 * RunServer is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * RunServer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RunServer. if not, see <http://www.gnu.org/licenses/>.
 */

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
#include "config.h"

const int DEBUG = 0;

//record system call
//标志：是否记录系统系统调用
static char record_call = 0;

char work_dir[1024];

static char lang_ext[3][8] = { "c", "cc", "java"};
static int time_lmt = 5, mem_lmt = 128, lang;//单位是second and MB

//获取文件大小
long get_file_size(const char * filename){
    struct stat f_stat;
    if (stat(filename, &f_stat) == -1){
        return 0;
    }
    return (long) f_stat.st_size;
}

//将log信息写入oj_home/log/client.log文件
void write_log(const char *fmt, ...){
    va_list ap;
    char buffer[4096];
    sprintf(buffer,"%s/client.log", getenv("LOG_PATH"));
    
    FILE *fp = fopen(buffer, "a+");
    if (fp == NULL){
        fprintf(stderr, "openfile error!\n");
    }
    
    va_start(ap, fmt);
    vsprintf(buffer, fmt, ap);
    fprintf(fp, "%s\n", buffer);
    
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

//初始化系统调用限制表
void init_syscalls_limits(int lang){
    int i;
    memset(call_counter, 0, sizeof(call_counter));
    if (DEBUG){
        write_log("init_call_counter:%d", lang);
    }
    
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



void find_next_nonspace(int & c1, int & c2, FILE *& f1, FILE *& f2, int & ret){
    // Find the next non-space character or \n.
    while ((isspace(c1)) || (isspace(c2))){
        if (c1 != c2){
            if (c2 == EOF){
                do{
                    c1 = fgetc(f1);
                }while (isspace(c1));
                continue;
            }else if (c1 == EOF){
                do{
                    c2 = fgetc(f2);
                }while (isspace(c2));
                continue;
            }else if ((c1 == '\r' && c2 == '\n')){
                c1 = fgetc(f1);
            }else if ((c2 == '\r' && c1 == '\n')){
                c2 = fgetc(f2);
            }else{
                ret = JudgePE;
            }
        }
        if (isspace(c1)){
            c1 = fgetc(f1);
        }if (isspace(c2)){
            c2 = fgetc(f2);
        }
    }
}


/*
 * translated from ZOJ judger r367
 * http://code.google.com/p/zoj/source/browse/trunk/judge_client/client/text_checker.cc#25
 *
 */
//比较用户输出和标准数据
int compare(const char *file1, const char *file2)
{
    int ret = JudgeAC;
    int c1,c2;
    FILE * f1, *f2 ;
    f1 = fopen(file1, "r");
    f2 = fopen(file2, "r");
    if (!f1 || !f2)
    {
        ret = JudgeRE;
    }
    else
        for (;;)
        {
            // Find the first non-space character at the beginning of line.
            // Blank lines are skipped.
            c1 = fgetc(f1);
            c2 = fgetc(f2);
            find_next_nonspace(c1, c2, f1, f2, ret);
            // Compare the current line.
            for (;;)
            {
                // Read until 2 files return a space or 0 together.
                while ((!isspace(c1) && c1) || (!isspace(c2) && c2))
                {
                    if (c1 == EOF && c2 == EOF)
                    {
                        goto end;
                    }
                    if (c1 == EOF || c2 == EOF)
                    {
                        break;
                    }
                    if (c1 != c2)
                    {
                        // Consecutive non-space characters should be all exactly the same
                        ret = JudgeWA;
                        goto end;
                    }
                    c1 = fgetc(f1);
                    c2 = fgetc(f2);
                }
                find_next_nonspace(c1, c2, f1, f2, ret);
                if (c1 == EOF && c2 == EOF)
                {
                    goto end;
                }
                if (c1 == EOF || c2 == EOF)
                {
                    ret = JudgeWA;
                    goto end;
                }
                
                if ((c1 == '\n' || !c1) && (c2 == '\n' || !c2))
                {
                    break;
                }
            }
        }
end:
    // if(ret==JudgeWA)make_diff_out(f1,f2,c1,c2,file1);
    if (f1)
        fclose(f1);
    if (f2)
        fclose(f2);
    return ret;
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

//运行编译后的程序
void run_solution(char *infile, int &usedtime){
    nice(19);
    
    // open the files
    freopen(infile, "r", stdin);
    freopen("user.out", "w", stdout);
    freopen("error.out", "a+", stderr);
    if(DEBUG){
        printf("%s\n", infile);
    }
    // run me
    if (lang != LangJava){
        chroot(work_dir);
    }
    
    
    struct rlimit LIM; // time limit, file limit& memory limit
    // time limit
    
    LIM.rlim_cur = (time_lmt - usedtime / 1000) + 1;
    LIM.rlim_max = LIM.rlim_cur;

    setrlimit(RLIMIT_CPU, &LIM);
    alarm(0);
    alarm(time_lmt*10);
    
    // file limit
    LIM.rlim_max = STD_F_LIM + STD_MB;
    LIM.rlim_cur = STD_F_LIM;
    setrlimit(RLIMIT_FSIZE, &LIM);
    
    // proc limit
    if(lang == LangJava){  //java
        LIM.rlim_cur=LIM.rlim_max = 1000;
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
        execl("/usr/bin/java", "/usr/bin/java", "-Xms128M", "-Xms512M",  "-Djava.security.manager", "-Djava.security.policy=./java.policy", "-DONLINE_JUDGE=true", "Main", (char *)NULL );
    }

    exit(0);
}

//评判用户 solution 的结果
void judge_solution(
    int &judge_flag, int usedtime, int topmemory, char *outfile, char *userfile){

    if (usedtime > time_lmt * 1000){
        judge_flag = JudgeTLE;
        return;
    }
    
    if(topmemory > mem_lmt * STD_MB){
        judge_flag = JudgeMLE;
        return;
    }
    
    // compare 标准输出和用户输出
    judge_flag = compare(outfile, userfile);
    
}

int get_page_fault_mem(struct rusage & ruse, pid_t & pidApp){
    //java use pagefault
    int m_minflt;
    m_minflt = ruse.ru_minflt * getpagesize();
    
    if (DEBUG){
        int m_vmpeak = get_proc_status(pidApp, "VmPeak:");
        int m_vmdata = get_proc_status(pidApp, "VmData:");
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
void watch_solution(
        pid_t pidApp, char *userfile, char *outfile, int &judge_flag, int &topmemory, int &usedtime){
    // parent
    int tempmemory;
    
    if (DEBUG){
        printf("pid=%d judging\n", pidApp);
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
            
            judge_flag = JudgeMLE;
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        
        int tmptime = usedtime + (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
        tmptime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);
        if(tmptime >= time_lmt*1000){
            judge_flag = JudgeTLE;
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        
        if (WIFEXITED(status)){
            break;
        }
        
        if (get_file_size("error.out")){
            judge_flag = JudgeRE;
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }
        
        if (get_file_size(userfile) > get_file_size(outfile) * 2+1024){
            judge_flag = JudgeOLE;
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
                    judge_flag = JudgeTLE;
                    break;
                case SIGXFSZ:
                    judge_flag = JudgeOLE;
                    break;
                default:
                    judge_flag = JudgeRE;
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
                    judge_flag = JudgeTLE;
                    break;
                case SIGXFSZ:
                    judge_flag = JudgeOLE;
                    break;
                default:
                    judge_flag = JudgeRE;
            }
            print_runtimeerror(strsignal(sig));
            break;
        }
        
        // check the system calls
        ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);
        
        if (!record_call&&call_counter[reg.REG_SYSCALL] == 0) {   //do not limit JVM syscall for using different JVM
            judge_flag = JudgeRE;
            char error[BUFFER_SIZE];
            sprintf(error,"[ERROR] A Not allowed system call! callid:%llu\n",reg.REG_SYSCALL);
            write_log(error);
            print_runtimeerror(error);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
        }else if(record_call){
            call_counter[reg.REG_SYSCALL] = 1;
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

//准备测试文件
int prepare_files(char * filename, char * infile, char * outfile, char * userfile){
    int l = strlen(filename);
    
    // 检查是否为 .in 文件
    if (l <= 3 || strcmp(filename + l - 3, ".in") != 0){
        return 0;
    }
    
    // 得到 .in 文件前缀
    char  fname[BUFFER_SIZE];
    strncpy(fname, filename,  l - 3);
    fname[l-3] = 0;
    
    sprintf(infile, "%s.in", fname);
    sprintf(outfile, "%s.out", fname);
    sprintf(userfile, "user.out");
}

//参数初始化
void init_parameters(int argc, char **argv){
    if (argc < 4){
        fprintf(stderr, "Usage:%s [language] [time limit] [memory limit] [run path]\n", argv[0]);
        exit(1);
    }
    
    lang = atoi(argv[1]);
    time_lmt = atoi(argv[2]);
    mem_lmt = atoi(argv[3])*1024/STD_MB*8;
    sprintf(work_dir,"%s", argv[4]);
    
    //java is lucky
    if (lang == LangJava){
        // the limit for java
        time_lmt = time_lmt * 2;
        mem_lmt = mem_lmt * 2;
        execute_cmd( "cp /etc/java-7-openjdk/security/java.policy %s/java.policy", work_dir);
    }
    
    //never bigger than judged set value;
    if (time_lmt > 30 || time_lmt < 1){
        time_lmt = 30;
    }
    
    if (mem_lmt > 1024 || mem_lmt < 1){
        mem_lmt = 1024;
    }
}

//输出用户程序的所用系统调用及调用次数
void print_call_array(){
    write_log("int LANG_%sV[256]={",LANG_NAME);
    int i=0;
    for (i = 0; i<call_array_size; i++){
        if(call_counter[i]){
            write_log("%d,",i);
        }
    }
    write_log("0};\n");
    
    write_log("int LANG_%sC[256]={",LANG_NAME);
    for (i = 0; i<call_array_size; i++){
        if(call_counter[i]){
            write_log("HOJ_MAX_LIMIT,");
        }
    }
    write_log("0};\n");
}

//judger 程序入口
int main(int argc, char** argv){
    
    init_parameters(argc, argv);
    
    if (DEBUG){
        printf("time: %d mem: %d\n", time_lmt, mem_lmt);
    }
    
    // cd work_dir
    if(chdir(work_dir) == -1){
        write_log("change work_dir %s failed\n", work_dir);
        exit(JudgeNA);
    }
    
    
    // begin run
    char fullpath[BUFFER_SIZE];
    getcwd(fullpath, BUFFER_SIZE);// the fullpath of data dir
    
    // open DIRs
    DIR *dp;
    dirent *dirp;
    
    if ((dp = opendir(fullpath)) == NULL){
        write_log("No such dir:%s!\n", fullpath);
        exit(JudgeNA);
    }
    
    int judge_flag = JudgeAC;
    int usedtime = 0, topmemory = 0;
    int count = 0;
    
    // read files and run
    for (; ( judge_flag == JudgeAC )&& (dirp = readdir(dp)) != NULL;){
        
        char infile[BUFFER_SIZE];
        char outfile[BUFFER_SIZE];
        char userfile[BUFFER_SIZE];
        
        if (prepare_files(dirp->d_name, infile, outfile, userfile) == 0){
            continue;
        }
        
        init_syscalls_limits(lang);
        
        if(DEBUG){
            printf("%s\n",infile);
        }
        
        pid_t pidApp = fork();
        if (pidApp == 0){
            run_solution(infile, usedtime);
            exit(0);
        }else{
            watch_solution(pidApp, userfile, outfile, judge_flag, topmemory, usedtime);
            
            if(judge_flag == JudgeAC){
                judge_solution(judge_flag, usedtime, topmemory, outfile, userfile);
            }
            count += 1;
        }
    }

    if(!count){
        judge_flag = JudgeNA;
    }
    
    if(judge_flag == JudgeTLE){
        usedtime = time_lmt*1000; // s to ms
    }
    
    write_log("result = %d usedtime = %d topmemory = %d", judge_flag, usedtime, topmemory);
    
    if(record_call){
        print_call_array();
    }
    
    printf("%d %d %d", judge_flag, usedtime, topmemory);
    
    if (judge_flag == JudgeAC) {
        exit(0);
    } else if (judge_flag == JudgeNA) {
        exit(3);
    }
    
    exit(judge_flag);
}
