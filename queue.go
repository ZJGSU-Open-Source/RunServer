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
	WaittingQueue *list.List
	SyncControll  Sync
)

func init() {
	pf, _ := os.Create("log")
	logger = log.New(pf, "", log.Lshortfile|log.Ltime)
	WaittingQueue = list.New()
}

func main() {
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

	SyncControll.AddQueue(info)
}

func LoadJson(r io.Reader, v interface{}) (err error) {
	err = json.NewDecoder(r).Decode(v)
	return
}

type Info struct {
	Sid     int
	Time    int
	Memory  int
	Rejudge bool
}

type Sync struct {
	sync.Locker
}

func (s *Sync) AddQueue(info *Info) {
	s.Lock()
	defer s.Unlock()

	WaittingQueue.PushBack(info)
}

func (s *Sync) GetFrontAndRemove() (info *Info) {
	s.Lock()
	defer s.Unlock()

	info, _ = WaittingQueue.Front().Value.(*Info)
	WaittingQueue.Remove(WaittingQueue.Front())
	return
}

func (s *Sync) IsEmpty() bool {
	s.Lock()
	defer s.Unlock()

	return WaittingQueue.Len() == 0

}

func JudgeForever() {
	for {
		if SyncControll.IsEmpty() == false {
			info := SyncControll.GetFrontAndRemove()
			judgeOne(*info)
		}
		time.Sleep(1 * time.Second)
	}
}
