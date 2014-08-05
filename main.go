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
	"time"
)

func main() {
	var sid = flag.Int("sid", -1, "solution id")
	var timeLimit = flag.Int("time", -1, "time limit")
	var memoryLimit = flag.Int("memory", -1, "memory limit")
	flag.Parse()
	log.Println("asdasd")
	judge(*sid, *memoryLimit, *timeLimit)
}

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

func judge(sid, memorylimit, timelimit int) {
	response, err := http.Post(config.PostHost+"/solution/detail/sid/"+strconv.Itoa(sid), "application/json", nil)
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
	one.SJudge(memorylimit, timelimit, "sample.in", "sample.out")
	if one.Judge == config.JudgeAC {
		one.SJudge(memorylimit, timelimit, "test.in", "test.out")
	}

	action := "submit"
	if one.Judge == config.JudgeAC {
		action = "solve"
	}
	///count if the problem has been solved
	response, err = http.Post(config.PostHost+"/solution/count/pid/"+strconv.Itoa(one.Pid)+"/uid/"+one.Uid+"/action/solve", "application/json", nil)
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
	response, err = http.Post(config.PostHost+"/user/record/uid/"+one.Uid+"/action/"+action, "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}

	response, err = http.Post(config.PostHost+"/problem/record/pid/"+strconv.Itoa(one.Pid)+"/action/"+action, "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}

	reader, err := PostReader(&one)
	if err != nil {
		log.Println(err)
		return
	}
	response, err = http.Post(config.PostHost+"/solution/update/sid/"+strconv.Itoa(one.Sid), "application/json", reader)
	defer response.Body.Close()
	if err != nil {
		log.Println(err)
		return
	}
}

func compare(std_out, user_out string) int {
	if std_out == user_out {
		return config.JudgeAC
	} else {
		return config.JudgeWA
	}
}

/*
一段c程序，获得pid程序的运行内存，示例：m_vmpeak = get_proc_status(pid, "VmPeak:");
int get_proc_status(int pid, const char * mark)
{
    FILE * pf;
    char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
    int ret = 0;
    sprintf(fn, "/proc/%d/status", pid);
    pf = fopen(fn, "r");
    int m = strlen(mark);
    while (pf && fgets(buf, BUFFER_SIZE - 1, pf))
    {

        buf[strlen(buf) - 1] = 0;
        if (strncmp(buf, mark, m) == 0)
        {
            sscanf(buf + m + 1, "%d", &ret);
        }
    }
    if (pf)
        fclose(pf);
    return ret;
}
*/

func (this *solution) SJudge(memorylimit, timelimit int, infilename, outfilename string) {
	log.Println("run solution")
	this.Judge = config.JudgePD
	pwd, err := os.Getwd()
	os.Chdir("/home/data/ACM/" + strconv.Itoa(this.Pid))//be careful.
	defer os.Chdir(pwd)
	std_in, std_out_f := os.Stdin, os.Stdout
	std_in, err = os.Open(infilename)
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

	//begin write code to file
	fl, err := os.Open("test.cpp")
	for err == nil {
		fl.Close()
		fl, err = os.Open("test.cpp")
	}
	codefile, err := os.Create("test.cpp")
	defer func() {
		codefile.Close()
		os.Remove("test.cpp")
	}()
	_, err = codefile.WriteString(this.Code)
	if err != nil {
		log.Println("source code writing to file failed")
	}
	//end write code to file

	//begin compile source code
	var out bytes.Buffer
	cmd := exec.Command("g++", "test.cpp", "-o", "Main", "-O2", "-Wall", "-lm", "--static", "-DONLINE_JUDGE") //compile
	err = cmd.Run()
	if err != nil {
		log.Println(err)
		this.Judge = config.JudgeCE //compiler error
		return
	}
	defer os.Remove("Main")
	//end compile source code

	target, err := os.Open("Main") //Memroy of the target
	defer target.Close()
	t_info, err := target.Stat()
	if err != nil {
		log.Println(err)
		return
	}
	if this.Memory = int(t_info.Size()) / (1024 * 1024); this.Memory > memorylimit {
		this.Judge = config.JudgeMLE
		return
	}

	//begin run
	channel := make(chan int)
	//process_id := make(chan int)
	defer close(channel)
	bcmd := exec.Command("./Main")
	bcmd.Stdin = std_in
	bcmd.Stdout = &out
	bcmd.Start()
	go func() {
		t_time := time.Now()
		err := bcmd.Wait()
		this.Time = int(time.Since(t_time).Seconds() * 1000)
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
			this.Time = bcmd.ProcessState.SystemTime() + bcmd.ProcessState.UserTime()
			user_out := out.String()
			this.Judge = compare(std_out, user_out)
		}
	case <-time.After(time.Second * 1):
		bcmd.Process.Kill()
		<-channel
		this.Judge = config.JudgeTLE
		this.Time = timelimit
	}
	return
}
