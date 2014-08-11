package main

import (
	"RunServer/config"
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

var logger *log.Logger

func init() {
	pf, _ := os.Create("log")
	logger = log.New(pf, "", log.Lshortfile|log.Ltime)
}

func main() {
	var sid = flag.Int("sid", -1, "solution id")
	var timeLimit = flag.Int("time", -1, "time limit")
	var memoryLimit = flag.Int("memory", -1, "memory limit")
	flag.Parse()

	response, err := http.Post(config.PostHost+"/solution/detail/sid/"+strconv.Itoa(*sid), "application/json", nil)
	if err != nil {
		logger.Println(err)
		return
	}
	defer response.Body.Close()

	var one solution

	if response.StatusCode == 200 {
		err = LoadJson(response.Body, &one)
		if err != nil {
			logger.Println(err)
			return
		}
	}

	cmd := exec.Command("mkdir", "../run/"+strconv.Itoa(one.Sid))
	cmd.Run()

	cmd = exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(one.Pid), "../run/"+strconv.Itoa(one.Sid))
	cmd.Run()
	defer os.RemoveAll("../run/" + strconv.Itoa(one.Sid))

	workdir := "../run/" + strconv.Itoa(one.Sid) + "/" + strconv.Itoa(one.Pid)
	logger.Println("workdir is ", workdir)
	one.files(workdir)
	one.judge(*memoryLimit, *timeLimit, workdir)
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

func (this *solution) judge(memoryLimit, timeLimit int, workdir string) {

	this.compile(workdir)
	if this.Judge != config.JudgeCE {
		this.Judge = config.JudgeRJ
		this.update()
		this.RunJudge(memoryLimit, timeLimit, workdir)
	}

	action := "submit"
	if this.Judge == config.JudgeAC {
		action = "solve"
	}

	///count if the problem has been solved
	response, err := http.Post(config.PostHost+"/solution/count/pid/"+strconv.Itoa(this.Pid)+"/uid/"+this.Uid+"/action/solve", "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		logger.Println(err)
		return
	}

	c := make(map[string]int)
	if response.StatusCode == 200 {
		err = LoadJson(response.Body, &c)
		if err != nil {
			logger.Println(err)
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
		logger.Println(err)
		return
	}

	response, err = http.Post(config.PostHost+"/problem/record/pid/"+strconv.Itoa(this.Pid)+"/action/"+action, "application/json", nil)
	defer response.Body.Close()
	if err != nil {
		logger.Println(err)
		return
	}

	this.update()
}

func (this *solution) compile(workdir string) {
	cmd := exec.Command("./compiler", strconv.Itoa(this.Language), workdir)
	cmd.Run()
	if cmd.ProcessState.String() != "exit status 0" {
		this.Judge = config.JudgeCE //compiler error
	}
}

func (this *solution) files(workdir string) {
	//begin write code to file
	var codefilename string
	if this.Language == config.LanguageC {
		codefilename = workdir + "/Main.c"
	} else if this.Language == config.LanguageCPP {
		codefilename = workdir + "/Main.cc"
	} else if this.Language == config.LanguageJAVA {
		codefilename = workdir + "/Main.java"
	}

	codefile, err := os.Create(codefilename)
	defer codefile.Close()

	_, err = codefile.WriteString(this.Code)
	if err != nil {
		logger.Println("source code writing to file failed")
	}
	//end write code to file
}

func (this *solution) RunJudge(memorylimit, timelimit int, workdir string) {
	logger.Println("run solution")

	var out bytes.Buffer
	cmd := exec.Command("./runner", strconv.Itoa(this.Pid), strconv.Itoa(this.Language), strconv.Itoa(timelimit), strconv.Itoa(memorylimit), workdir)
	cmd.Stdout = &out
	cmd.Run()
	sp := strings.Split(out.String(), " ")
	var err error
	this.Judge, err = strconv.Atoi(sp[0])
	if err != nil {
		logger.Println(err)
		logger.Println(this.Judge)
	}
	this.Time, _ = strconv.Atoi(sp[1])
	this.Memory, _ = strconv.Atoi(sp[2])
	this.Memory = this.Memory / (1024 * 8) //b->KB
	logger.Println(this.Time)
	logger.Println(this.Memory)
}

func (this *solution) update() {
	reader, err := PostReader(this)
	if err != nil {
		logger.Println(err)
		return
	}
	response, err := http.Post(config.PostHost+"/solution/update/sid/"+strconv.Itoa(this.Sid), "application/json", reader)
	defer response.Body.Close()
	if err != nil {
		logger.Println(err)
		return
	}
}
