package main

import (
	"GoOnlineJudge/model"
	"RunServer/config"
	"bytes"
	"errors"
	"io/ioutil"
	"os"
	"os/exec"
	"strconv"
	"strings"
	"vjudger"
)

var ErrCompile = errors.New("compile error")

type ZJGSUJudger struct {
	token   string
	workdir string
	time    int
	mem     int
} //ZJGSUJudger implements vjudger.Vjudger interface.

func (z *ZJGSUJudger) Init(user *vjudger.User) error {
	z.token = "ZJGSU"

	cmd := exec.Command("mkdir", "../run/"+strconv.Itoa(user.Sid))
	cmd.Run()
	defer os.RemoveAll("../run/" + strconv.Itoa(user.Sid))

	z.workdir = "../run/" + strconv.Itoa(user.Sid) + "/" + strconv.Itoa(user.Vid)
	logger.Println("workdir is ", z.workdir)

	z.files(user, z.workdir)
	return nil
}

func (z *ZJGSUJudger) Match(token string) bool {
	if z.token == token || token == "" {
		return true
	}
	return false
}

//Get problem Info
func (z *ZJGSUJudger) Login(user *vjudger.User) error {
	proModel := &model.ProblemModel{}
	pro, _ := proModel.Detail(user.Vid)
	z.time = pro.Time
	z.mem = pro.Memory
	return nil
}

func (z *ZJGSUJudger) files(user *vjudger.User, workdir string) {
	var codefilename string

	switch user.Lang {
	case config.LanguageC:
		codefilename = workdir + "/Main.c"
	case config.LanguageCPP:
		codefilename = workdir + "/Main.cc"
	case config.LanguageJAVA:
		codefilename = workdir + "/Main.java"
	}

	codefile, err := os.Create(codefilename)
	defer codefile.Close()

	_, err = codefile.WriteString(user.Code)
	if err != nil {
		logger.Println("source code writing to file failed")
	}
}

func (z *ZJGSUJudger) Submit(user *vjudger.User) error {
	z.compile(user)
	if user.Result != config.JudgeCE {
		user.Result = config.JudgeRJ
		logger.Println("compiler success")
		user.UpdateSolution()

		cmd := exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(user.Vid), "../run/"+strconv.Itoa(user.Sid))
		cmd.Run()
	} else {
		b, err := ioutil.ReadFile(z.workdir + "/ce.txt")
		if err != nil {
			logger.Println(err)
		}
		user.CE = string(b)
		logger.Println("compiler error")
		return ErrCompile
	}
	return nil
}

func (z *ZJGSUJudger) GetStatus(user *vjudger.User) {
	logger.Println("run solution")

	var out bytes.Buffer
	cmd := exec.Command("./runner", strconv.Itoa(user.Vid), strconv.Itoa(user.Lang), strconv.Itoa(z.time), strconv.Itoa(z.mem), z.workdir)
	cmd.Stdout = &out
	cmd.Run()

	sp := strings.Split(out.String(), " ")
	var err error
	user.Result, err = strconv.Atoi(sp[0])
	if err != nil {
		logger.Println(err)
		logger.Println(user.Result)
	}
	user.Time, _ = strconv.Atoi(sp[1])
	user.Mem, _ = strconv.Atoi(sp[2])
	user.Mem = user.Mem / 1024 //b->Kb
	logger.Println(user.Time, "ms")
	logger.Println(user.Mem, "Kb")
}

func (z *ZJGSUJudger) compile(user *vjudger.User) {
	cmd := exec.Command("./compiler", strconv.Itoa(user.Lang), z.workdir)
	cmd.Run()
	if cmd.ProcessState.String() != "exit status 0" {
		user.Result = config.JudgeCE //compiler error
	}
}
