#include "lib.h"
#include "Query.cpp"
#include "LanguageModel.cpp"
#include "Utility.cpp"
#include "Corpus.cpp"
using namespace std;
struct Data
{
    string tid;
	string timestamp;
    string content;
    double dirichletScore;
    double JMScore;
    double voteScore;  // 0.5 * (dirichletScore + JMScore)
};

struct Term
{
    string content;
    double weight;
};
bool cmpTerm(const Term &t1, const Term &t2) {
    return t1.weight > t2.weight;
}

bool cmp1(const Data &d1, const Data &d2) {
    return d1.dirichletScore > d2.dirichletScore;
}

bool cmp2(const Data &d1, const Data &d2) {
    return d1.JMScore > d2.JMScore;
}

bool cmp3(const Data &d1, const Data &d2) {
    return d1.voteScore > d2.voteScore;
}

bool isValidTerm(string term) {
    const char *temp = term.c_str();
    for (int i = 0; i < term.length(); i ++) {
        if (!(temp[i]<='9'&&temp[i]>='0'))
          return true;
    }
    return false;
}


void normTime(struct tm &t) {
	if (t.tm_hour == 24) {
		t.tm_mday += 1;
		t.tm_hour = 0;
	}
	if (t.tm_hour == -1) {
		t.tm_mday -= 1;
		t.tm_hour = 23;
	}
}

vector<Data> getNovelListForQuery(int qid, int startDay, int endDay, string year, string month, string scenarioADir) {
	vector<Data> novelList;
	stringstream sqid;
	sqid << qid;
	stringstream spath;
	spath << year << "-" << month << "-" << startDay;
	struct tm cur_time;
	strptime(spath.str().c_str(),"%Y-%m-%d",&cur_time);
	int curDay = startDay;
	while (curDay <= endDay) {
		char res_suffix[100];
		strftime(res_suffix,100,"%Y-%m-%d",&cur_time);
		string resPath = scenarioADir + sqid.str() + "/statuses.scoreA." + res_suffix;
		if (access(resPath.c_str(),F_OK) == 0) {
			ifstream resFile(resPath.c_str());
			string line;
			while (getline(resFile, line)) {
				vector<string> score_info = Utility::split(line, '\t');
				struct Data data;
				data.tid = score_info[0];
				data.timestamp = score_info[1];
				data.content = score_info[2];
				data.dirichletScore = atof(score_info[3].c_str());
				data.JMScore = atof(score_info[4].c_str());
				data.voteScore = atof(score_info[5].c_str());
				novelList.push_back(data);
			}
		}
		curDay += 1;
		cur_time.tm_mday += 1;
	}
	cout << "["<<qid<<"]"<<" has " << novelList.size() << " novel tweets between day["
		<< startDay << "," << endDay <<"]"<< endl;
	return novelList;
}

int main(int argc, char **argv) {
	if (argc < 16) {
        cout << "argv[1]: relevanceThreshold" << endl;
		cout << "argv[2]: novelThreshold" << endl;
		cout << "argv[3]: startEvalDay" << endl;
        cout << "argv[4]: endEvalDay" << endl;
        cout << "argv[5]: initCorpusStartDay" << endl;
        cout << "argv[6]: initCorpusEndDay" << endl; 
        cout << "argv[7]: queryFile" << endl;
        cout << "argv[8]: scoredDir" << endl;
        cout << "argv[9]: year" << endl;
        cout << "argv[10]: month" << endl;  // e.g. 07
        cout << "argv[11]: scenarioADir" << endl;
		cout << "argv[12]: parsedDir" << endl;
		cout << "argv[13]: startHour" << endl;
		cout << "argv[14]: adaptiveThresholdDir" << endl;
        cout << "argv[15]: runMode" << endl;
        return -1;
    }
	double relevanceThreshold = atof(argv[1]);
	double novelThreshold = atof(argv[2]);
	int startEvalDay = atoi(argv[3]);
    int endEvalDay = atoi(argv[4]);
    int initCorpusStartDay = atoi(argv[5]);
    int initCorpusEndDay = atoi(argv[6]);
    string queryFileName = string(argv[7]);
    string scoredDir = string(argv[8]);
    string year = string(argv[9]);
    string month = string(argv[10]);
    string scenarioADir = string(argv[11]);
	string parsedDir = string(argv[12]);
	int startHour = atoi(argv[13]);
	string adaptiveThresholdDir = string(argv[14]);
	int runMode = atoi(argv[15]);  // runMode (1: adaptive 2:manual 3:adaptive for curDay A)
	// init corpus with a date range
	Corpus collection(parsedDir, year, month,initCorpusStartDay,initCorpusEndDay);
	// init query for all interest profile
	ifstream queryFile(queryFileName.c_str());
    string line;
    vector<Query> queryVector;
	int qid = 0;
    while (getline(queryFile, line)) {
		vector<string> query_vector = Utility::split(line, '\t');
		Query query(query_vector[0],query_vector[1]);
        queryVector.push_back(query);
    }
	queryFile.close();
	// init language model
	LanguageModel lm;
    int curHour = startHour;
    stringstream start_time_stream;
    start_time_stream << year << "-" << month << "-" << startEvalDay << "-" << curHour;

    string curScoredFilePath;
    string nextScoreFilePath;
    char cur_suffix[100];
    char next_suffix[100];
	struct tm cur_time;
	strptime(start_time_stream.str().c_str(),"%Y-%m-%d-%H",&cur_time);
    
	int curDay = startEvalDay;
	// record pushed novel tweets number for each qid of curDay
	map<int, int> curDayNovelTweetCountMap;
	map<int, double> curDayRelevanceThresholdMap;
	map<int, double> curDayNovelThresholdMap;
	while (true) {
		strftime(cur_suffix, 100, "%Y-%m-%d-%H",&cur_time);
        cur_time.tm_hour += 1;
		normTime(cur_time);
		strftime(next_suffix, 100, "%Y-%m-%d-%H",&cur_time);
        cur_time.tm_hour -= 1;
		normTime(cur_time);
		char curDayString[100];
		strftime(curDayString, 100, "%d",&cur_time);
		if (curDay != atoi(curDayString)) {// change days
			curDayNovelTweetCountMap.clear();
			curDayRelevanceThresholdMap.clear();
			curDayNovelThresholdMap.clear();
		}
		curDay = atoi(curDayString);
		if (curDay > endEvalDay) break; // exit the program

		// check whether to update the threshold
		char curDay_suffix[100];
		strftime(curDay_suffix,100,"%Y%m%d",&cur_time);
		string curDayAdaptiveThresholdFilePath;
		map<int, string> adaptiveThresholdMap;


		if (runMode == 1 || runMode == 2 || runMode == 3) {  // adaptive or manual threshold
			if (runMode == 1 || runMode == 3)
		 		curDayAdaptiveThresholdFilePath = adaptiveThresholdDir + string(curDay_suffix) + ".B";
		 	else 
		 		curDayAdaptiveThresholdFilePath = adaptiveThresholdDir + string(curDay_suffix) + ".M";
		 	while (true) {
			 	if (access(curDayAdaptiveThresholdFilePath.c_str(), F_OK) == 0) {
			 		string line;
			 		ifstream thresholdFile(curDayAdaptiveThresholdFilePath.c_str());
			 		while (getline(thresholdFile, line)) {
			 			vector<string> thresholdVector = Utility::split(line, '\t');
			 			//string thresholdInfo = "";
			 			// for (int i = 3; i < thresholdVector.size(); i ++) {
			 			// 	if (thresholdInfo == "")
			 			// 		thresholdInfo = thresholdVector[i];
			 			// 	else
			 			// 		thresholdInfo += "\t" + thresholdVector[i];
			 			// }
			 			curDayRelevanceThresholdMap.insert(make_pair(atoi(thresholdVector[1].c_str()),atof(thresholdVector[3].c_str())));
			 			curDayNovelThresholdMap.insert(make_pair(atoi(thresholdVector[1].c_str()),atof(thresholdVector[4].c_str())));
			 			//adaptiveThresholdMap.insert(make_pair(atoi(thresholdVector[1].c_str()), thresholdInfo));
			 		}
			 		thresholdFile.close();
			 		break;
			 	}
			 	else {
			 		cout << "Miss adaptive Threshold File: " << curDayAdaptiveThresholdFilePath.c_str() << endl;
			 		if (curDay == startEvalDay) {  // first eval day could have no adaptive threshold
			 			break;
			 		}
			 		else {
			 			cout << "Sleep to wait for adaptive threshold file:" << curDayAdaptiveThresholdFilePath.c_str()<<endl;
			 			sleep(30);   // sleep to wait
			 		}
			 	}
		 	}
		}

		// get the output path and open output stream
		//vector<string> outputScorePath;
		//vector<ofstream *> outputScoreStream;
		for (int qid = 0; qid < queryVector.size(); qid ++) {
			stringstream sqid;
			sqid << qid+1;
			// init novel tweet count map
			if (curDayNovelTweetCountMap.find(qid+1) == curDayNovelTweetCountMap.end()) {
				curDayNovelTweetCountMap.insert(make_pair(qid+1,0));
			}

			// if novel tweets > 10, skip
			if (curDayNovelTweetCountMap[qid+1] >= 10) continue;

			if (runMode == 1 || runMode == 2 || runMode == 3) {  // use adaptive threshold for each query
				if (curDay > startEvalDay) { // skip start eval day
					//vector<string> thresholdVector = Utility::split(adaptiveThresholdMap[qid+1],'\t');
					// relevanceThreshold = atof(thresholdVector[0].c_str());
					// novelThreshold = atof(thresholdVector[1].c_str());
					relevanceThreshold = curDayRelevanceThresholdMap[qid+1];
					//novelThreshold = curDayNovelThresholdMap[qid+1];  // use uniform novel threshold
				}

				else if (curDay == startEvalDay) {  // adaptive tuned threshold for start day
					relevanceThreshold = atof(argv[1]) + curDayNovelTweetCountMap[qid+1]*0.01 - (cur_time.tm_hour) * 0.02 / 23.0;
				}
			}
			else if (curDay == startEvalDay) {  // adaptive tuned threshold for start day
				relevanceThreshold = atof(argv[1]) + curDayNovelTweetCountMap[qid+1]*0.01 - (cur_time.tm_hour) * 0.02 / 23.0;
			}
			// check whether condition satified
			string curScoredFilePath = scoredDir + sqid.str()+"/statuses.scored." + string(cur_suffix);
			string nextScoredFilePath = scoredDir + sqid.str()+"/statuses.scored." + string(next_suffix);
			//File *fh;
			//while ((fh=fopen(nextScoredFilePath.c_str(),"rb")) == NULL) { // current hour scored file is not ready
			while (access(nextScoredFilePath.c_str(),F_OK) != 0) { // current hour scored file is not ready
				cout << "[" << qid+1 <<"]current scored file " << curScoredFilePath.c_str() << " is not ready, next file ("
					<< nextScoredFilePath.c_str() << ") does not exist..." << endl;
				sleep(30);
				continue;
			} 
			//fh.close();
			//cout << "["<<qid+1<<"]curScoredFilePath:" << curScoredFilePath.c_str() << " is ready..."<< endl;
			//cout << "["<<qid+1<<"]nextScoredFilePath:" << nextScoredFilePath.c_str() << " is ready..."<< endl;
			string line;
			ifstream curCorpusFile(curScoredFilePath.c_str());

			char suffix[100];
			strftime(suffix,100,"%Y-%m-%d",&cur_time);
			string curScoreAFilePath = scenarioADir + sqid.str() + "/statuses.scoreA." + suffix;
			//outputScorePath.push_back(curScoreAFilePath);
			/*ofstream *scoreStream = new ofstream();
			scoreStream->open(curScoreAFilePath.c_str(),ios::app);
			outputScoreStream.push_back(scoreStream);*/
			ofstream resFile(curScoreAFilePath.c_str(),ios::app);
			// load previous novel files
			vector<Data> novelList = getNovelListForQuery(qid+1,startEvalDay,curDay,year,month,scenarioADir);
			while (getline(curCorpusFile, line)) {  // calculate the score
				vector<string> tweet_vector = Utility::split(line, '\t');
				clock_t start, finish;
				double duration;
				start = clock();
				struct Data currentVector;
				currentVector.tid = tweet_vector[0];
				currentVector.timestamp = tweet_vector[1];
				currentVector.content = tweet_vector[2];
				currentVector.dirichletScore = atof(tweet_vector[3].c_str());
				currentVector.JMScore = atof(tweet_vector[4].c_str());
				currentVector.voteScore = atof(tweet_vector[5].c_str());
				//cout << currentVector.JMScore << endl;
				if (atof(tweet_vector[5].c_str()) > relevanceThreshold) {
					double maxScore = 0;
					int maxIndex = 0;
					for (int j = 0; j < novelList.size(); j ++) {
						Query query1(sqid.str(), tweet_vector[2]);
						Query query2(sqid.str(), novelList[j].content);
						Tweet t1(tweet_vector[0],tweet_vector[1],tweet_vector[2]);
						Tweet t2(novelList[j].tid,novelList[j].timestamp,novelList[j].content);
						double max = 0;
                		double min = -20;
						double s1 = (lm.computeKLDivergence(query1, t2, collection)-min)/(max-min);
						double s2 = (lm.computeKLDivergence(query2, t1, collection)-min)/(max-min);
						double s3 = (lm.computeKLDivergenceWithJM(query1, t2, collection)-min)/(max-min);
						double s4 = (lm.computeKLDivergenceWithJM(query2, t1, collection)-min)/(max-min);
						double score = (s1 + s2 + s3 + s4) / 4.0;
						if (score > maxScore) {
							maxScore = score;
							maxIndex = j;
						}
					}
					if (novelList.size() == 0) {
							finish = clock();
							duration = (double)(finish-start) / CLOCKS_PER_SEC;
							int addTime = (int)duration;
							if (curDay > startEvalDay) {  // skip first day 19
								cout << "[" << qid << "]first novel tweet generated" << endl;
								resFile << currentVector.tid<<"\t"<<currentVector.timestamp
								<<"\t"<<currentVector.content<<"\t"<<currentVector.dirichletScore
								<<"\t"<<currentVector.JMScore<<"\t"<<currentVector.voteScore
								<<"\t"<<currentVector.tid<<"\t"<<maxScore<<"\t"<<addTime<<"\n";
							}
							struct Data newData(currentVector);
							novelList.push_back(newData);

							// update novel count map
							curDayNovelTweetCountMap[qid+1] += 1;
							if (runMode == 3) {  // update threshold
								curDayNovelThresholdMap[qid+1] -= 0.02;
							}
							if (curDayNovelTweetCountMap[qid+1] >= 10) break;
					}
					else if (maxScore < novelThreshold) { // new novel tweet
						finish = clock();
						duration = (double)(finish-start) / CLOCKS_PER_SEC;
						int addTime = (int)duration;
						if (curDay > startEvalDay) {  // skip first day 19
							resFile << currentVector.tid<<"\t"<<currentVector.timestamp
							<<"\t"<<currentVector.content<<"\t"<<currentVector.dirichletScore
							<<"\t"<<currentVector.JMScore<<"\t"<<currentVector.voteScore
							<<"\t"<<novelList[maxIndex].tid<<"\t"<<maxScore<<"\t"<<addTime<<"\n";
						}
						struct Data newData(currentVector);
						novelList.push_back(newData);

						// update novel count map
						curDayNovelTweetCountMap[qid+1] += 1;
						if (runMode == 3) {  // update threshold
								curDayNovelThresholdMap[qid+1] -= 0.02;
						}
						if (curDayNovelTweetCountMap[qid+1] >= 10) break;
					}
				}
			} 
			resFile.close();
			curCorpusFile.close();
		}
		
		// update time
        cur_time.tm_hour += 1;
        normTime(cur_time);
		/*cur_time.tm_hour += 1;
		next_time.tm_hour += 1;
		if (cur_time.tm_hour == 24) {
			cur_time.tm_mday += 1;
			cur_time.tm_hour = 0;
			if (cur_time.tm_mday == 32) {
				cur_time.tm_mday = 1;
				cur_time.tm_mon += 1;
			}
		}
		if (next_time.tm_hour == 24) {
			next_time.tm_mday += 1;
			next_time.tm_hour = 0;
			if (next_time.tm_mday == 32) {
				next_time.tm_mday = 1;
				next_time.tm_mon += 1;
			}
		} */
		
		//reset
		/*outputScorePath.clear();
		for (int qid = 0; qid < queryVector.size(); qid ++) {
			outputScoreStream[qid]->close();
		}
		outputScoreStream.clear();*/
		
	}
}
