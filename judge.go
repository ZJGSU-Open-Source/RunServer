package main

import (
	"GoOnlineJudge/model"

	"RunServer/config"
	"fmt"
	"log"
	"os"
	"os/exec"
	"strconv"
	"time"
)

type solution struct {
	Vid        int
	OJ         string
	SubmitTime time.Time
	model.Solution
}

func (s *solution) GetResult() int {
	return s.Judge
}

func (s *solution) SetResult(result int) {
	s.Judge = result
}

func (s *solution) SetResource(time int, Mem int, length int) {
	s.Time = time
	s.Memory = Mem
	s.Length = length
}

func (s *solution) SetErrorInfo(text string) {
	s.Error = text
}

func (s *solution) GetSubmitTime() time.Time {
	return s.SubmitTime
}

func (s *solution) SetSubmitTime(submitTime time.Time) {
	s.SubmitTime = submitTime
}

func (s *solution) GetCode() string {
	return s.Code
}

func (s *solution) GetOJ() string {
	return s.OJ
}

func (s *solution) GetLang() int {
	return s.Language
}

func (s *solution) GetVid() int {
	return s.Vid
}

func (s *solution) GetSid() int {
	return s.Sid
}

func (s *solution) GetPid() int {
	return s.Pid
}

func (s *solution) Init(info Info) {
	logger.Println(info)

	solutionModel := model.SolutionModel{}
	sol, err := solutionModel.Detail(info.Sid)
	if err != nil {
		logger.Println(err)
		return
	}
	s.Solution = *sol

	s.OJ = info.OJ
	s.Vid = info.Pid
}

func (this *solution) UpdateSim() {

	var sim, Sim_s_id int

	if this.Judge == config.JudgeAC && this.Module == config.ModuleC { //当为练习或竞赛时检测
		sim, Sim_s_id = this.get_sim(this.Sid, this.Language)
	}

	this.Sim = sim
	this.Sim_s_id = Sim_s_id

	this.UpdateSolution()
}

func (this *solution) UpdateRecord() {
	solutionModel := model.SolutionModel{}
	qry := make(map[string]string)
	qry["module"] = strconv.Itoa(config.ModuleP)
	qry["action"] = "submit"
	qry["pid"] = strconv.Itoa(this.Pid)

	submit, _ := solutionModel.Count(qry)

	qry["action"] = "solve"
	solve, _ := solutionModel.Count(qry)

	proModel := model.ProblemModel{}
	err := proModel.Record(this.Pid, solve, submit)
	if err != nil {
		logger.Println(err)
	}

	qry["action"] = "submit"
	qry["uid"] = this.Uid
	delete(qry, "pid")
	delete(qry, "module")
	submit, _ = solutionModel.Count(qry)

	solvelist, err := solutionModel.Achieve(this.Uid)
	if err != nil {
		logger.Println(err)
	}
	solve = len(solvelist)

	userModel := model.UserModel{}
	err = userModel.Record(this.Uid, solve, submit)
	if err != nil {
		logger.Println(err)
	}
}

//get_sim 相似度检测，返回值为相似度和相似的id
func (this *solution) get_sim(Sid, Language int) (sim, Sim_s_id int) {
	var extension string

	if this.Language == config.LanguageC {
		extension = "c"
	} else if this.Language == config.LanguageCPP {
		extension = "cc"
	} else if this.Language == config.LanguageJAVA {
		extension = "java"
	}

	qry := make(map[string]string)
	qry["pid"] = strconv.Itoa(this.Pid)
	qry["action"] = "solve"

	solutionModel := model.SolutionModel{}
	list, err := solutionModel.List(qry)
	workdir := runPath + "/" + strconv.Itoa(this.Sid)
	log.Println(workdir)

	sim_test_dir := workdir + "/sim_test"

	cmd := exec.Command("mkdir", "-p", sim_test_dir)
	err = cmd.Run()
	if err != nil {
		logger.Println(err)
		return
	}
	defer os.RemoveAll(workdir)

	codefile, err := os.Create(sim_test_dir + "/../Main." + extension)
	defer codefile.Close()

	_, err = codefile.WriteString(this.Code)
	if err != nil {
		logger.Println("source code writing to file failed")
	}

	var count int

	for i := range list {
		sid := list[i].Sid

		sol, err := solutionModel.Detail(sid)
		if sid < this.Sid && err == nil {
			filepath := sim_test_dir + "/" + strconv.Itoa(sid) + "." + extension

			codefile, err := os.Create(filepath)
			defer codefile.Close()

			_, err = codefile.WriteString(sol.Code)
			if err != nil {
				logger.Println("source code writing to file failed")
			}
			count++
		}
	}

	cmd = exec.Command("sim.sh", sim_test_dir, extension)

	if err = cmd.Run(); err != nil {
		return
	}
	defer os.Remove("./sim")

	if _, err := os.Stat("./sim"); err == nil {

		simfile, err := os.Open("./sim")

		if err != nil {
			logger.Println("sim file open error")
			return
		}
		defer simfile.Close()

		fmt.Fscanf(simfile, "%d %d", &sim, &Sim_s_id)
	}
	return sim, Sim_s_id
}

//UpdateSolution 更新判题结果
func (this *solution) UpdateSolution() {
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
	ori.Sim = this.Sim
	ori.Sim_s_id = this.Sim_s_id
	ori.Error = this.Error

	err = solutionModel.Update(sid, *ori)
	if err != nil {
		logger.Println(err)
		return
	}
}
