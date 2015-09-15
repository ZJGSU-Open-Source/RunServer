// RunServer is Judger and dispatcher for ZJGSU New OJ http://acm.zjgsu.edu.cn

// Copyright (C) 2013-2014 -  ZJGSU OSC[https://github.com/ZJGSU-Open-Source/]

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
package main

import (
	"vjudger"

	"container/list"
	"encoding/json"
	"io"
	"log"
	"net/http"
	"os"
	"sync"
	"time"
)

var (
	logger        *log.Logger
	waittingQueue *list.List
	SyncControll  *Sync
	runPath       string
	dataPath      string
)

func init() {
	runPath = os.Getenv("RUN_PATH")
	dataPath = os.Getenv("DATA_PATH")

	logger = log.New(os.Stdout, "", log.Lshortfile|log.Ltime|log.Ldate)
	waittingQueue = list.New()

	log.Println(runPath)
}

func main() {
	SyncControll = &Sync{}
	go JudgeForever()

	http.HandleFunc("/", Handler)
	http.ListenAndServe(":8888", nil)
}

func Handler(w http.ResponseWriter, r *http.Request) {
	var info = &Info{}
	err := LoadJson(r.Body, info)
	if err != nil {
		log.Println("load json err")
		return
	}
	logger.Printf("Sid %v, Rejudge %v\n", info.Sid, info.Rejudge)

	SyncControll.AddQueue(info)
}

func LoadJson(r io.Reader, v interface{}) (err error) {
	err = json.NewDecoder(r).Decode(v)
	return
}

//Info 判题必须信息
type Info struct {
	Sid     int
	Pid     int
	OJ      string
	Rejudge bool
}

// Sync 锁
type Sync struct {
	lock sync.Mutex
}

// 添加到队尾
func (s *Sync) AddQueue(info *Info) {
	s.lock.Lock()
	defer s.lock.Unlock()

	waittingQueue.PushBack(info)
}

// 获得并删除队头
func (s *Sync) GetFrontAndRemove() (info *Info) {
	s.lock.Lock()
	defer s.lock.Unlock()

	info, _ = waittingQueue.Front().Value.(*Info)
	waittingQueue.Remove(waittingQueue.Front())
	return
}

// 队列是否为空
func (s *Sync) IsEmpty() bool {
	s.lock.Lock()
	defer s.lock.Unlock()

	return waittingQueue.Len() == 0
}

//
func JudgeForever() {
	for {
		if SyncControll.IsEmpty() == false {
			info := SyncControll.GetFrontAndRemove()
			go Judge(*info) //并行判题
		}
		time.Sleep(1 * time.Second)
	}
}

var VJs = []vjudger.Vjudger{&ZJGSUJudger{}, &vjudger.HDUJudger{}, &vjudger.PKUJudger{}}

func Judge(info Info) {
	user := &solution{}
	user.Init(info)
	for _, vj := range VJs {
		if vj.Match(user.GetOJ()) {
			vj.Run(user)
			break
		}
	}
	user.UpdateSim()
	user.UpdateRecord()
}
