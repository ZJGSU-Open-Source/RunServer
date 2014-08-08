//This file if forked from https://github.com/siqiaochen/oj_client_hustoj/
//it hasn't been tested, so don't use it in your OJ.
//
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

static int DEBUG = 0;
static char oj_home[BUFFER_SIZE];
static int port_number;
static int max_running;
static int sleep_time;
static int java_time_bonus = 5;
static int java_memory_bonus = 512;
static char java_xms[BUFFER_SIZE];
static char java_xmx[BUFFER_SIZE];
static int sim_enable = 0;
static int use_max_time=0;

static int shm_run=0;

static char record_call=0;

//static int sleep_tmp;

static char lang_ext[4][8] = { "c", "cc", "pas", "java"};
//static char buf[BUFFER_SIZE];

long get_file_size(const char * filename){
    struct stat f_stat;
    if (stat(filename, &f_stat) == -1){
        return 0;
    }
    return (long) f_stat.st_size;
}

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

int isInFile(const char fname[]){
    int l = strlen(fname);
    if (l <= 3 || strcmp(fname + l - 3, ".in") != 0)
        return 0;
    else
        return l - 3;
}

int compare(const char *file1, const char *file2){
	return OJ_AC;
}

FILE * read_cmd_output(const char * fmt, ...){
    char cmd[BUFFER_SIZE];

    FILE *  ret =NULL;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    va_end(ap);
    if(DEBUG) printf("%s\n",cmd);
    ret = popen(cmd,"r");

    return ret;
}

int compile(int lang){
    int pid;

    const char * CP_C[] = { "gcc", "Main.c", "-o", "Main","-Wall", "-lm",
                            "--static", "-std=c99", "-DONLINE_JUDGE", NULL
                          };
    const char * CP_X[] = { "g++", "Main.cc", "-o", "Main", "-Wall",
                            "-lm", "--static","-std=c++0x", "-DONLINE_JUDGE", NULL
                          };
//      const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m", "Main.java",NULL };

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
        if (lang != 2){
            freopen("ce.txt", "w", stderr);
            //freopen("/dev/null", "w", stdout);
        }else{
            freopen("ce.txt", "w", stdout);
        }
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
        if (DEBUG)
            printf("compile end!\n");
        //exit(!system("cat ce.txt"));
        exit(0);
    }else{
        int status=0;

        waitpid(pid, &status, 0);
        if (DEBUG)
            printf("status=%d\n", status);
        return status;
    }

}

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
    if (pf)
        fclose(pf);
    return ret;
}



void prepare_files(char * filename, int namelen, char * infile, char * work_dir, char * outfile, char * userfile){
    //              printf("ACflg=%d %d check a file!\n",ACflg,solution_id);

    char  fname[BUFFER_SIZE];
    strncpy(fname, filename, namelen);
    fname[namelen] = 0;
    sprintf(infile, "%s/data/%s.in", oj_home, fname);
    execute_cmd("cp %s %s/data.in", infile, work_dir);
    execute_cmd("cp %s/data/*.dic %s/", oj_home,work_dir);

    sprintf(outfile, "%s/data/%s.out", oj_home, fname);
    sprintf(userfile, "%s/run/user.out", oj_home);
}


void run_solution(int & lang, char * work_dir, int & time_lmt, int & usedtime, int & mem_lmt){
    nice(19);
    // now the user is "judger"
    chdir(work_dir);
    // open the files
    freopen("data.in", "r", stdin);
    freopen("user.out", "w", stdout);
    freopen("error.out", "a+", stderr);
    // trace me
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    // run me
    if (lang != 3)
        chroot(work_dir);

    while(setgid(1536)!=0) sleep(1);
    while(setuid(1536)!=0) sleep(1);
    while(setresuid(1536, 1536, 1536)!=0) sleep(1);

//      char java_p1[BUFFER_SIZE], java_p2[BUFFER_SIZE];
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

    if(lang == LangC || lang ==LangCC){
        execl("./Main", "./Main", (char *)NULL);
    }else if(lang == LangJava){
//              sprintf(java_p1, "-Xms%dM", mem_lmt / 2);
//              sprintf(java_p2, "-Xmx%dM", mem_lmt);
        execl("/usr/bin/java", "/usr/bin/java", java_xms,java_xmx,
              "-Djava.security.manager",
              "-Djava.security.policy=./java.policy", "Main", (char *)NULL);
    }
    //sleep(1);
    exit(0);
}
int fix_java_mis_judge(char *work_dir, int & ACflg, int & topmemory, int mem_lmt){
    int comp_res = OJ_AC;
    if (DEBUG)
        execute_cmd("cat %s/error.out", work_dir);
    comp_res = execute_cmd("grep 'java.lang.OutOfMemoryError'  %s/error.out",
                           work_dir);

    if (!comp_res){
        printf("JVM need more Memory!");
        ACflg = OJ_ML;
        topmemory = mem_lmt * STD_MB;
    }
    comp_res = execute_cmd("grep 'java.lang.OutOfMemoryError'  %s/user.out",
                           work_dir);

    if (!comp_res){
        printf("JVM need more Memory or Threads!");
        ACflg = OJ_ML;
        topmemory = mem_lmt * STD_MB;
    }
    comp_res = execute_cmd("grep 'Could not create'  %s/error.out", work_dir);

    if (!comp_res){
        printf("jvm need more resource,tweak -Xmx(OJ_JAVA_BONUS) Settings");
        ACflg = OJ_RE;
        //topmemory=0;
    }
    return comp_res;
}

void judge_solution(int & ACflg, int & usedtime, int time_lmt,
                    char * infile, char * outfile, char * userfile, int & PEflg,
                    int lang, char * work_dir, int & topmemory, int mem_lmt, double num_of_test){
    //usedtime-=1000;
    int comp_res;
    if (ACflg == OJ_AC && usedtime > time_lmt * 1000*(use_max_time?1:num_of_test))
        ACflg = OJ_TL;
    if(topmemory>mem_lmt * STD_MB) ACflg=OJ_ML; //issues79
    // compare
    if (ACflg == OJ_AC){
        comp_res = compare(outfile, userfile);
        if (comp_res == OJ_WA){
            ACflg = OJ_WA;
            if (DEBUG)
                printf("fail test %s\n", infile);
        }
        else if (comp_res == OJ_PE)
            PEflg = OJ_PE;
        ACflg = comp_res;
    }
    //jvm popup messages, if don't consider them will get miss-WrongAnswer
    if (lang == 3 && ACflg != OJ_AC){
        comp_res = fix_java_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
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
void print_runtimeerror(char * err){
    FILE *ferr=fopen("error.out","a+");
    fprintf(ferr,"Runtime Error:%s\n",err);
    fclose(ferr);
}
void clean_session(pid_t p){
    //char cmd[BUFFER_SIZE];
    const char *pre = "ps awx -o \"\%p \%P\"|grep -w ";
    const char *post = " | awk \'{ print $1  }\'|xargs kill -9";
    execute_cmd("%s %d %s",pre,p,post);
    execute_cmd("ps aux |grep \\^judge|awk '{print $2}'|xargs kill");
}

void watch_solution(pid_t pidApp, char * infile, int & ACflg,
                    char * userfile, char * outfile, int lang,
                    int & topmemory, int mem_lmt, int & usedtime, int time_lmt,
                    int & PEflg, char * work_dir){
    // parent
    int tempmemory;

    if (DEBUG)
        printf("pid=%d judging %s\n", pidApp, infile);

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
            if (DEBUG)
                printf("out of memory %d\n", topmemory);
            if (ACflg == OJ_AC){
                ACflg = OJ_ML;
            }
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }

        if (WIFEXITED(status)){
            break;
        }
        if ((lang < 4) && get_file_size("error.out")){
            ACflg = OJ_RE;
            //addreinfo(solution_id);
            ptrace(PTRACE_KILL, pidApp, NULL, NULL);
            break;
        }

        if (get_file_size(userfile) > get_file_size(outfile) * 2+1024){
            ACflg = OJ_OL;
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

            if (ACflg == OJ_AC){
                switch (exitcode){
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    ACflg = OJ_TL;
                    break;
                case SIGXFSZ:
                    ACflg = OJ_OL;
                    break;
                default:
                    ACflg = OJ_RE;
                }
                print_runtimeerror(strsignal(exitcode));
            }
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
            if (ACflg == OJ_AC){
                switch (sig){
                case SIGCHLD:
                case SIGALRM:
                    alarm(0);
                case SIGKILL:
                case SIGXCPU:
                    ACflg = OJ_TL;
                    break;
                case SIGXFSZ:
                    ACflg = OJ_OL;
                    break;
                default:
                    ACflg = OJ_RE;
                }
                print_runtimeerror(strsignal(sig));
            }
            break;
        }
        /*     comment from http://www.felix021.com/blog/read.php?1662

        WIFSTOPPED: return true if the process is paused or stopped while ptrace is watching on it
        WSTOPSIG: get the signal if it was stopped by signal
         */

        // check the system calls
        ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);

        if (!record_call&&call_counter[reg.REG_SYSCALL] == 0) {   //do not limit JVM syscall for using different JVM
            ACflg = OJ_RE;
            char error[BUFFER_SIZE];
            sprintf(error,"[ERROR] A Not allowed system call! callid:%ld\n",
                    reg.REG_SYSCALL);
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

    //clean_session(pidApp);
}
void clean_workdir(char * work_dir ){
    if (DEBUG){
        execute_cmd("mv %s/* %slog/", work_dir, work_dir);
    }else{
        execute_cmd("umount %s/proc", work_dir);
        execute_cmd("rm -Rf %s/*", work_dir);

    }

}

void init_parameters(int argc, char ** argv, int & lang, int & time_lmt, int & mem_lmt){
    if (argc < 3){
        fprintf(stderr, "Usage:%s [language] [time limit] [memory limit].\n", argv[0]);
        fprintf(stderr, "Multi:%s [language] [time limit] [memory limit] [base path],\n",
                argv[0]);
        fprintf(stderr,
                "Debug:%s solution_id runner_id judge_base_path debug.\n",
                argv[0]);
        //exit(1);

        lang = 0;
        time_lmt = 10;
        mem_lmt = 10;
        strcpy(oj_home, "/home/judger");
        chdir(oj_home);
        DEBUG=1;
        return;
    }
    DEBUG = (argc > 6);
    record_call=(argc > 7);
    if(argc > 5){
        strcpy(LANG_NAME,argv[5]);
    }
    if (argc > 4)
        strcpy(oj_home, argv[4]);
    else
        strcpy(oj_home, "/home/judger");

    chdir(oj_home); // change the dir// init our work

    lang = atoi(argv[1]);
    time_lmt = atoi(argv[2]);
    mem_lmt = atoi(argv[3]);
}
void mk_shm_workdir(char * work_dir){
    char shm_path[BUFFER_SIZE];
    sprintf(shm_path,"/dev/shm/hustoj/%s",work_dir);
    execute_cmd("mkdir -p %s",shm_path);
    execute_cmd("rm -rf %s",work_dir);
    execute_cmd("ln -s %s %s/",shm_path,oj_home);
    execute_cmd("chown judge %s ",shm_path);
    execute_cmd("chmod 755 %s ",shm_path);
    //sim need a soft link in shm_dir to work correctly
    sprintf(shm_path,"/dev/shm/hustoj/%s/",oj_home);
    execute_cmd("ln -s %s/data %s",oj_home,shm_path);

}
int count_in_files(char * dirpath){
    const char  * cmd="ls -l %s/*.in|wc -l";
    int ret=0;
    FILE * fjobs=read_cmd_output(cmd,dirpath);
    fscanf(fjobs,"%d",&ret);
    pclose(fjobs);

    return ret;
}

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

int main(int argc, char** argv){

    char work_dir[BUFFER_SIZE];
    int time_lmt, mem_lmt, lang, sim, sim_s_id,max_case_time=0;

    time_lmt=5;
    mem_lmt=128;

	//TODO. get problem limit
    init_parameters(argc, argv, lang, time_lmt, mem_lmt);
	
    //set work directory to start running & judging
    sprintf(work_dir, "%s/run/", oj_home);

    if(shm_run) mk_shm_workdir(work_dir);

    chdir(work_dir);
    if (!DEBUG)
        clean_workdir(work_dir);
    //java is lucky
    if (lang == 3){
        // the limit for java
        time_lmt = time_lmt + java_time_bonus;
        mem_lmt = mem_lmt + java_memory_bonus;
        // copy java.policy
        execute_cmd( "cp %s/etc/java0.policy %sjava.policy", oj_home, work_dir);

    }

    //never bigger than judged set value;
    if (time_lmt > 300 || time_lmt < 1)
        time_lmt = 300;
    if (mem_lmt > 1024 || mem_lmt < 1)
        mem_lmt = 1024;

    if (DEBUG)
        printf("time: %d mem: %d\n", time_lmt, mem_lmt);

    // begin compile
    // set the result to compiling
    int Compile_OK;

    Compile_OK = compile(lang);
    if (Compile_OK != 0){
        //update_solution(OJ_CE, 0, 0, 0,0, 0.0);
        if (!DEBUG)
            clean_workdir(work_dir);
        else
            write_log("compile error");
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
    sprintf(fullpath, "%s/data/", oj_home); // the fullpath of data dir



    // open DIRs
    DIR *dp;
    dirent *dirp;

    if ((dp = opendir(fullpath)) == NULL){

        write_log("No such dir:%s!\n", fullpath);
        exit(-1);
    }

    int ACflg, PEflg;
    ACflg = PEflg = OJ_AC;
    int namelen;
    int usedtime = 0, topmemory = 0;

    // read files and run
    double pass_rate=0.0;
    int num_of_test=0;
    int finalACflg=ACflg;

    for (; ( ACflg == OJ_AC )&& (dirp = readdir(dp)) != NULL;){

        namelen = isInFile(dirp->d_name); // check if the file is *.in or not
        if (namelen == 0)
            continue;

        prepare_files(dirp->d_name, namelen, infile, work_dir, outfile, userfile);
        init_syscalls_limits(lang);

        pid_t pidApp = fork();

        if (pidApp == 0){

            run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
        }else{

            num_of_test++;

            watch_solution(pidApp, infile, ACflg, userfile, outfile,
                           lang, topmemory, mem_lmt, usedtime, time_lmt
                           , PEflg, work_dir);


            judge_solution(ACflg, usedtime, time_lmt, infile,
                           outfile, userfile, PEflg, lang, work_dir, topmemory,
                           mem_lmt, num_of_test);
            if(use_max_time){
                max_case_time=usedtime>max_case_time?usedtime:max_case_time;
                usedtime=0;
            }
            //clean_session(pidApp);
        }
    }
    if (ACflg == OJ_AC && PEflg == OJ_PE)
        ACflg = OJ_PE;
    if (sim_enable && ACflg == OJ_AC &&(finalACflg==OJ_AC)){
        // do not compare similarity with other solutions
        //sim = get_sim(lang, sim_s_id);
        sim_s_id = 0;
    }else{
        sim = 0;
    }
    //if(ACflg == OJ_RE)addreinfo(solution_id);

    if(ACflg==OJ_RE){
        if(DEBUG) printf("add RE info ..... \n");
     //   addreinfo(solution_id);
    }
    if(use_max_time){
        usedtime=max_case_time;
    }
    if(ACflg == OJ_TL){
        usedtime=time_lmt*1000;
    }
  
    //update_solution(ACflg, usedtime, topmemory >> 10, sim,
        //                sim_s_id,0);
    if(ACflg==OJ_WA){
        if(DEBUG) printf("add diff info of ..... \n");
        //adddiffinfo();
    }
    //update_user();
    //update_problem();
    clean_workdir(work_dir);

    if (DEBUG)
        write_log("result=%d", ACflg);
    if(record_call){
        print_call_array();
    }

    return 0;
}
