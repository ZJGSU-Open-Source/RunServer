package main

import (
	"RunServer/config"
	"bufio"
	"bytes"
	"encoding/json"
	"flag"
	"io"
	"log"
	"net/http"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"syscall"
	"time"
)

type solution struct {
	Sid int `json:"sid"bson:"sid"`

	Pid      int    `json:"pid"bson:"pid"`
	Uid      string `json:"uid"bson:"uid"`
	Judge    int    `json:"judge"bson:"judge"`
	Time     int    `json:"time"bson:"time"`
	Memory   int    `json:"memory"bson:"memory"`
	Length   int    `json:"length"bson:"length"`
	Language int    `json:"language"bson:"language"`

	Module int `json:"module"bson:"module"`
	Mid    int `json:"mid"bson:"mid"`

	Code string `json:"code"bson:"code"`

	Status int    `json:"status"bson:"status"`
	Create string `json:"create"bson:"create"`
}

func main() {
	var sid = flag.Int("sid", -1, "solution id")
	var timeLimit = flag.Int("time", -1, "time limit")
	var memoryLimit = flag.Int("memory", -1, "memory limit")
	flag.Parse()

	response, err := http.Post(config.PostHost+"/solution/detail/sid/"+strconv.Itoa(*sid), "application/json", nil)
	if err != nil {
		log.Println(err)
		return
	}
	defer response.Body.Close()

	var one solution

	if response.StatusCode == 200 {
		err = LoadJson(response.Body, &one)
		if err != nil {
			log.Println(err)
			return
		}
	}

	pwd, err := os.Getwd()

	cmd := exec.Command("mkdir", "../run/"+strconv.Itoa(one.Sid))
	cmd.Run()

	cmd = exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(one.Pid), "../run/"+strconv.Itoa(one.Sid))
	cmd.Run()
	defer os.RemoveAll("../run/" + strconv.Itoa(one.Sid))

	os.Chdir("../run/" + strconv.Itoa(one.Sid) + "/" + strconv.Itoa(one.Pid)) //
	defer os.Chdir(pwd)

	one.files()
	one.judge(*memoryLimit, *timeLimit)
}

func PostReader(i interface{}) (r io.Reader, err error) {
	b, err := json.Marshal(i)
	if err != nil {
		return
	}
	r = strings.NewReader(string(b))
	return
}

func LoadJson(r io.Reader, i interface{}) (err error) {
	err = json.NewDecoder(r).Decode(i)
	return
}

func (this *solution) judge(memoryLimit, timeLimit int) {

	this.compile()
	if this.Judge != config.JudgeCE {
		this.Judge = config.JudgeRJ
		this.update()
		//TODO:should get all test files.
		//this.RunJudge(memorylimit, timelimit, "sample.in", "sample.out")
		//if one.Judge == config.JudgeAC {
		this.RunJudge(memoryLimit, timeLimit, "sample.in", "sample.out")
		//}
	}

	action := "submit"
	if this.Judge == config.JudgeAC {
		action = "solve"
	}

	///count if the problem has been solved
	response, err := http.Post(config.PostHost+"/solution/count/pid/"+strconv.Itoa(this.Pid)+"/uid/"+this.Uid+"/action/solve", "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}

	c := make(map[string]int)
	if response.StatusCode == 200 {
		err = LoadJson(response.Body, &c)
		if err != nil {
			log.Println(err)
			return
		}

	}
	///end count

	if c["count"] >= 1 && action == "solve" { //当结果正确且不是第一次提交，只记录为提交而不记录为solve
		action = "submit"
	}
	response, err = http.Post(config.PostHost+"/user/record/uid/"+this.Uid+"/action/"+action, "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}

	response, err = http.Post(config.PostHost+"/problem/record/pid/"+strconv.Itoa(this.Pid)+"/action/"+action, "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}

	this.update()
}

func compare(std_out, user_out string) int {
	if std_out == user_out {
		return config.JudgeAC
	} else {
		return config.JudgeWA
	}
}

//一段c程序，获得pid程序的运行内存，示例：m_vmpeak = get_proc_status(pid, "VmPeak:");
// func get_proc_status(pid int, mark string) int {
//

// 	var buf bytes.Buffer
// 	var ret int

// 	fn := "/proc/" + strconv.Itoa(pid) + "/status"
// 	pf := os.Open(fn)
//	defer pf.close()
//
// 	m := len(mark)
// 	for pf && fgets(buf, BUFFER_SIZE-1, pf) {
// 		buf[strlen(buf)-1] = 0
// 		if strncmp(buf, mark, m) == 0 {
// 			sscanf(buf+m+1, "%d", &ret)
// 		}
// 	}

// 	return ret
// }

func (this *solution) compile() {
	//begin compile source code

	var cmd *exec.Cmd
	if this.Language == config.LanguageC {
		cmd = exec.Command("gcc", "Main.c", "-o", "Main", "-O2", "-Wall", "-lm", "--static", "-std=c99", "-DONLINE_JUDGE")
	} else if this.Language == config.LanguageCPP {
		cmd = exec.Command("g++", "Main.cpp", "-o", "Main", "-O2", "-Wall", "-lm", "--static", "-DONLINE_JUDGE") //compile
	} else if this.Language == config.LanguageJAVA {
		cmd = exec.Command("javac", "-J-Xms32m", "-J-Xmx256m", "Main.java")
	}
	err := cmd.Run()
	if err != nil {
		log.Println(err)
		this.Judge = config.JudgeCE //compiler error
		return
	}

	//end compile source code
}

func (this *solution) files() {
	//begin write code to file
	var codefilename string
	if this.Language == config.LanguageC {
		codefilename = "Main.c"
	} else if this.Language == config.LanguageCPP {
		codefilename = "Main.cpp"
	} else if this.Language == config.LanguageJAVA {
		codefilename = "Main.java"
	}

	codefile, err := os.Create(codefilename)
	defer codefile.Close()

	_, err = codefile.WriteString(this.Code)
	if err != nil {
		log.Println("source code writing to file failed")
	}
	//end write code to file
}

func (this *solution) RunJudge(memorylimit, timelimit int, infilename, outfilename string) {
	log.Println("run solution")
	this.Judge = config.JudgePD

	std_in, std_out_f := os.Stdin, os.Stdout
	std_in, err := os.Open(infilename)
	if err != nil {
		log.Println("Open std_in file failed")
		return
	}
	defer std_in.Close()

	//begin read the std_out text
	var std_out string
	std_out_f, err = os.Open(outfilename)
	if err != nil {
		log.Println("open std_out file failed")
		return
	} else {
		reader := bufio.NewReader(std_out_f)
		for true {
			line, _, err := reader.ReadLine()
			if err == io.EOF {
				std_out = std_out + string(line) //+ "\n"
				break
			} else if err != nil {
				break
			}
			std_out = std_out + string(line) + "\n"
		}
	}
	defer std_out_f.Close()
	//end read the std_out text

	var out bytes.Buffer
	//begin run
	channel := make(chan int)
	//process_id := make(chan int)
	defer close(channel)
	bcmd := exec.Command("./Main")
	bcmd.Stdin = std_in
	bcmd.Stdout = &out
	go func() {
		bcmd.Start()
		err := bcmd.Wait()
		if err != nil {
			channel <- config.JudgeRE //runtime error
			return
		}
		channel <- config.JudgeCP
	}()
	//end run

	select {
	case res := <-channel:
		if res == config.JudgeCP {
			rusage, _ := bcmd.ProcessState.SysUsage().(*syscall.Rusage)

			this.Time = int((rusage.Stime.Usec + rusage.Utime.Usec) / 1000)
			if this.Time >= timelimit {
				this.Time = timelimit
				this.Judge = config.JudgeTLE
				return
			}
			this.Memory = int(rusage.Minflt) * syscall.Getpagesize() / 1024

			if this.Memory >= memorylimit {
				this.Judge = config.JudgeMLE
			}

			user_out := out.String()
			this.Judge = compare(std_out, user_out)
		}
	case <-time.After(time.Second * time.Duration(timelimit/1000)):
		bcmd.Process.Kill()
		<-channel
		this.Judge = config.JudgeTLE
		this.Time = timelimit
	}
	return
}

func (this *solution) update() {
	reader, err := PostReader(this)
	if err != nil {
		log.Println(err)
		return
	}
	response, err := http.Post(config.PostHost+"/solution/update/sid/"+strconv.Itoa(this.Sid), "application/json", reader)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}
}
