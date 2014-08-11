package config

var PostHost = "http://127.0.0.1:8888"

const (
	JudgePD  = 0  //Pending
	JudgeRJ  = 1  //Running & judging
	JudgeCE  = 2  //Compile Error
	JudgeAC  = 3  //Accepted
	JudgeRE  = 4  //Runtime Error
	JudgeWA  = 5  //Wrong Answer
	JudgeTLE = 6  //Time Limit Exceeded
	JudgeMLE = 7  //Memory Limit Exceeded
	JudgeOLE = 8  //Output Limit Exceeded
	JudgePE  = 9  //Presentation Error
	JudgeNA  = 10 //None
)

const (
	LanguageNA   = 0 //None
	LanguageC    = 1 //C
	LanguageCPP  = 2 //C++
	LanguageJAVA = 3 //Java
)
