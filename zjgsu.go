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

const ZJGSUToken = "ZJGSU"

type ZJGSUJudger struct {
	token   string
	workdir string
	time    int
	mem     int
} //ZJGSUJudger implements vjudger.Vjudger interface.

func (z *ZJGSUJudger) Init(user vjudger.UserInterface) error {
	z.token = ZJGSUToken

	z.workdir = "../run/" + strconv.Itoa(user.GetSid()) + "/" + strconv.Itoa(user.GetVid())
	logger.Println("workdir is ", z.workdir)

	cmd := exec.Command("mkdir", "-p", z.workdir)
	cmd.Run()
	defer os.RemoveAll("../run/" + strconv.Itoa(user.GetSid()))

	z.files(user, z.workdir)
	return nil
}

func (z *ZJGSUJudger) Match(token string) bool {

	if ZJGSUToken == token || token == "" {
		return true
	}
	return false
}

//Get problem Info
func (z *ZJGSUJudger) Login(user vjudger.UserInterface) error {
	proModel := &model.ProblemModel{}
	logger.Println(user.GetVid())
	pro, _ := proModel.Detail(user.GetVid())
	z.time = pro.Time
	z.mem = pro.Memory
	return nil
}

func (z *ZJGSUJudger) files(user vjudger.UserInterface, workdir string) {
	var codefilename string

	switch user.GetLang() {
	case config.LanguageC:
		codefilename = workdir + "/Main.c"
	case config.LanguageCPP:
		codefilename = workdir + "/Main.cc"
	case config.LanguageJAVA:
		codefilename = workdir + "/Main.java"
	}

	codefile, err := os.Create(codefilename)
	defer codefile.Close()

	_, err = codefile.WriteString(user.GetCode())
	if err != nil {
		logger.Println("source code writing to file failed")
	}
}

func (z *ZJGSUJudger) Submit(user vjudger.UserInterface) error {
	z.compile(user)

	if user.GetResult() != config.JudgeCE {
		user.SetResult(config.JudgeRJ)
		logger.Println("compile success")
		user.UpdateSolution()

		cmd := exec.Command("cp", "-r", "../ProblemData/"+strconv.Itoa(user.GetVid()), "../run/"+strconv.Itoa(user.GetSid()))
		cmd.Run()
	} else {
		b, err := ioutil.ReadFile(z.workdir + "/ce.txt")
		if err != nil {
			logger.Println(err)
		}
		user.SetErrorInfo(string(b))
		logger.Println("compiler error")
		return ErrCompile
	}
	return nil
}

func (z *ZJGSUJudger) GetStatus(user vjudger.UserInterface) error {
	logger.Println("run solution")

	var out bytes.Buffer
	cmd := exec.Command("./runner", strconv.Itoa(user.GetVid()), strconv.Itoa(user.GetLang()), strconv.Itoa(z.time), strconv.Itoa(z.mem), z.workdir)
	cmd.Stdout = &out
	cmd.Run()

	sp := strings.Split(out.String(), " ")
	var err error
	Result, err := strconv.Atoi(sp[0])

	if err != nil {
		logger.Println(err)
		logger.Println(Result)
	}
	user.SetResult(Result)
	Time, _ := strconv.Atoi(sp[1])
	Mem, _ := strconv.Atoi(sp[2])
	Mem = Mem / 1024 //b->Kb
	user.SetResource(Time, Mem, len(user.GetCode()))
	return nil
}

func (z *ZJGSUJudger) Run(u vjudger.UserInterface) error {
	for _, apply := range []func(vjudger.UserInterface) error{z.Init, z.Login, z.Submit, z.GetStatus} {
		if err := apply(u); err != nil {
			logger.Println(err)
			return err
		}
	}
	return nil
}

func (z *ZJGSUJudger) compile(user vjudger.UserInterface) {
	cmd := exec.Command("./compiler", strconv.Itoa(user.GetLang()), z.workdir)
	cmd.Run()
	if cmd.ProcessState.String() != "exit status 0" {
		user.SetResult(config.JudgeCE) //compiler error
	}
}
