//
// File:   compiler.cc
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
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <assert.h>

#include "config.h"

int main(int argc, char* argv[]){
    int pid;

    int lang = atoi(argv[1]);
    char *workdir = argv[2];

    chdir(workdir);
    
    const char * CP_C[] = { "gcc", "Main.c", "-o", "Main","-Wall", "-lm",
                            "--static", "-std=c99", "-DONLINE_JUDGE", NULL
                          };
    const char * CP_X[] = { "g++", "Main.cc", "-o", "Main", "-Wall",
                            "-lm", "--static","-std=c++0x", "-DONLINE_JUDGE", NULL
                          };
	const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m", "Main.java",NULL };

    pid = fork();
    if (pid == 0){
        struct rlimit LIM;
        LIM.rlim_max = 30;
        LIM.rlim_cur = 30;
        setrlimit(RLIMIT_CPU, &LIM); //longest compilation time is 30s.
        alarm(0);
        alarm(30);

        LIM.rlim_max = 100 * STD_MB;
        LIM.rlim_cur = 100 * STD_MB;
        setrlimit(RLIMIT_FSIZE, &LIM);

        LIM.rlim_max =  STD_MB<<11;
        LIM.rlim_cur =  STD_MB<<11;
        setrlimit(RLIMIT_AS, &LIM);
        
        freopen("ce.txt", "w", stderr);//record copmile error
        
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
            exit(-1);
        }
        exit(0);
    }else{
        int status=0;
        waitpid(pid, &status, 0);
        if(status == 0){
            exit(0);
        }else{
            exit(1);
        }
    }
}