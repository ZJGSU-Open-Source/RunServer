package main

import (
	"GoOnlineJudge/model"
	"GoOnlineJudge/model/class"
	"RunServer/config"
	"bytes"
	"encoding/json"
	"flag"
	"io"
	"log"
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

	Status int   `json:"status"bson:"status"`
	Create int64 `json:"create"bson:"create"`
}

type SolutionModel struct {
	class.Model
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

	solutionModel := model.SolutionModel{}
	solid, err := strconv.Atoi(strconv.Itoa(*sid))
	if err != nil {
		logger.Println(err)
		return
	}

	sol, err := solutionModel.Detail(solid)

	if err != nil {
		logger.Println(err)
		return
	}
	cmd := exec.Command("mkdir", "../run/"+strconv.Itoa(sol.Sid))
	cmd.Run()

	cmd = exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(sol.Pid), "../run/"+strconv.Itoa(sol.Sid))
	cmd.Run()

	workdir := "../run/" + strconv.Itoa(sol.Sid) + "/" + strconv.Itoa(sol.Pid)
	logger.Println("workdir is ", workdir)

	var one solution

	one.Code = sol.Code
	one.Judge = sol.Judge
	one.Create = sol.Create
	one.Language = sol.Language
	one.Length = sol.Length
	one.Memory = sol.Memory
	one.Mid = sol.Mid
	one.Module = sol.Module
	one.Pid = sol.Pid
	one.Sid = sol.Sid
	one.Status = sol.Status
	one.Time = sol.Time
	one.Uid = sol.Uid

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
		logger.Println("compiler success")
		this.update()
		this.RunJudge(memoryLimit, timeLimit, workdir)
	} else {
		logger.Println("compiler error")
	}

	action := "submit"
	if this.Judge == config.JudgeAC {
		action = "solve"

		///count if the problem has been solved
		solutionModel := model.SolutionModel{}
		qry := make(map[string]string)
		qry["uid"] = this.Uid
		qry["pid"] = strconv.Itoa(this.Pid)
		qry["action"] = "solve"
		c, err := solutionModel.Count(qry)

		if err != nil {
			logger.Println(err)
			return
		}
		///end count

		if c >= 1 && action == "solve" { //当结果正确且不是第一次提交，只记录为提交而不记录为solve
			action = "submit"
		}
	}

	userModel := model.UserModel{}
	err := userModel.Record(this.Uid, action)

	if err != nil {
		logger.Println(err)
		return
	}

	proModel := model.ProblemModel{}
	err = proModel.Record(this.Pid, action)

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
	sid, err := strconv.Atoi(strconv.Itoa(this.Sid))

	solutionModel := model.SolutionModel{}
	ori, err := solutionModel.Detail(sid)
	if err != nil {
		logger.Println(err)
		return
	}

	ori.Judge = this.Judge
	ori.Time = this.Time
	ori.Memory = this.Memory

	err = solutionModel.Update(sid, *ori)

	if err != nil {
		logger.Println(err)
		return
	}
}
