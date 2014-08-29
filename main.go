package main

import (
	"GoOnlineJudge/model"
	"GoOnlineJudge/model/class"
	"RunServer/config"
	"bytes"
	"flag"
	"log"
	"os"
	"os/exec"
	"strconv"
	"strings"
)

type solution struct {
	model.Solution
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
	var rejudge = flag.Int("rejudge", -1, "if rejudge")
	flag.Parse()

	solutionModel := model.SolutionModel{}
	solutionID, err := strconv.Atoi(strconv.Itoa(*sid))
	if err != nil {
		logger.Println(err)
		return
	}

	sol, err := solutionModel.Detail(solutionID)
	if err != nil {
		logger.Println(err)
		return
	}

	cmd := exec.Command("mkdir", "../run/"+strconv.Itoa(sol.Sid))
	cmd.Run()

	cmd = exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(sol.Pid), "../run/"+strconv.Itoa(sol.Sid))
	cmd.Run()
	defer os.RemoveAll("../run/" + strconv.Itoa(sol.Sid))

	workdir := "../run/" + strconv.Itoa(sol.Sid) + "/" + strconv.Itoa(sol.Pid)
	logger.Println("workdir is ", workdir)

	//var one *solution
	one := &solution{}

	one.Solution = *sol
	one.files(workdir)
	one.judge(*memoryLimit, *timeLimit, *rejudge, workdir)
}

func (this *solution) judge(memoryLimit, timeLimit, rejudge int, workdir string) {
	this.compile(workdir)
	if this.Judge != config.JudgeCE {
		this.Judge = config.JudgeRJ
		logger.Println("compiler success")
		this.update()
		this.RunJudge(memoryLimit, timeLimit, workdir)
	} else {
		logger.Println("compiler error")
	}

	solve, submit := 0, 1

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

	record := false
	if this.Judge == config.JudgeAC {
		if c == 0 {
			solve = 1
		} else if c >= 1 {
			solve = 0
		}
	} else {
		//here
		//c should be 1, however c == 0
		logger.Println(c)
		if c == 1 && rejudge != -1 {
			solve = -1
			record = true
		} else {
			solve = 0
		}
	}
	if rejudge != -1 {
		submit = 0
	}

	if record || rejudge == -1 {
		userModel := model.UserModel{}
		err = userModel.Record(this.Uid, solve, submit)

		if err != nil {
			logger.Println(err)
			return
		}

		proModel := model.ProblemModel{}
		err = proModel.Record(this.Pid, solve, submit)

		if err != nil {
			logger.Println(err)
			return
		}
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
	logger.Println(this.Time, "ms")
	logger.Println(this.Memory, "kB")
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
