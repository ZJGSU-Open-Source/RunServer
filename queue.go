package main

import (
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
)

func init() {
	pf, _ := os.Create("log")
	logger = log.New(pf, "", log.Lshortfile|log.Ltime|log.Ldate)
	waittingQueue = list.New()
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
	logger.Printf("Sid %v, Problem Memory %v, Problem Time %v,Rejudge %v\n", info.Sid, info.Memory, info.Time, info.Rejudge)
	SyncControll.AddQueue(info)
}

func LoadJson(r io.Reader, v interface{}) (err error) {
	err = json.NewDecoder(r).Decode(v)
	return
}

//Info 判题必须信息
type Info struct {
	Sid     int
	Time    int
	Memory  int
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
			judgeOne(*info)
		}
		time.Sleep(1 * time.Second)
	}
}
