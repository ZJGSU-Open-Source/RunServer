package config

var PostHost = "http://127.0.0.1:8888"

const (
	JudgeNA  = 0  //None
	JudgePD  = 1  //Pending
	JudgeRJ  = 2  //Running & judgingconst
	JudgeAC  = 3  //Accepted
	JudgeCE  = 4  //Compile Error
	JudgeRE  = 5  //Runtime Error
	JudgeWA  = 6  //Wrong Answer
	JudgeTLE = 7  //Time Limit Exceeded
	JudgeMLE = 8  //Memory Limit Exceeded
	JudgeOLE = 9  //Output Limit Exceeded
	JudgeCP  = 10 //wait to Compare Output
)

const (
	LanguageNA	= 0	//None
	LanguageC	= 1	//C
	LanguageCPP = 2 //C++
	LanguageJAVA= 3 //Java
)
