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

vector<Data> getNovelListForQuery(int qid, int startDay, int endDay,string year,string month,string scenarioBDir) {
	vector<Data> novelList;
	stringstream ss1;
	ss1 << qid;
	stringstream ss2;
	ss2 << year<<"-"<<month<<"-" << startDay;
	struct tm cur_time;
	strptime(ss2.str().c_str(),"%Y-%m-%d",&cur_time);
	int curDay = startDay;
	while (curDay < endDay) {
		char res_suffix[100];
		strftime(res_suffix,100,"%Y-%m-%d",&cur_time);
		string resPath = scenarioBDir + ss1.str() + "/statuses.scoreB." + res_suffix;
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
		else {
			cout << "InGetNovelList:[" << qid << "] scoreB file " << resPath.c_str() << " miss..." << endl;
		}
		curDay += 1;
		cur_time.tm_mday += 1;
	}
	cout << "["<<qid<<"]"<<" has " << novelList.size() << " novel tweets between day["
		<< startDay << "," << endDay << ")." << endl;
	return novelList;
}



int main(int argc, char **argv) {
	if (argc < 19) {
        cout << "argv[1]: relevanceThreshold" << endl;
		cout << "argv[2]: highRelevanceThreshold" << endl;
		cout << "argv[3]: novelThreshold" << endl;
		cout << "argv[4]: startEvalDay" << endl;
        cout << "argv[5]: endEvalDay" << endl;
        cout << "argv[6]: initCorpusStartDay" << endl;
        cout << "argv[7]: initCorpusEndDay" << endl;
        cout << "argv[8]: scoredDir" << endl;
        cout << "argv[9]: queryFile" << endl;
        cout << "argv[10]: year" << endl;
        cout << "argv[11]: month" << endl;
        cout << "argv[12]: scenarioBDir" << endl;
        cout << "argv[13]: parsedDir" << endl;
        cout << "argv[14]: topNDocument" << endl;
        cout << "argv[15]: topKTerm" << endl;
        cout << "argv[16]: adaptiveThresholdDir" << endl;
        cout << "argv[17]: runMode" << endl;
        cout << "argv[18]: topNForChoosingThreshold" << endl;
        return -1;
    }
	double relevanceThreshold = atof(argv[1]);
	double highRelevanceThreshold = atof(argv[2]);
	double novelThreshold = atof(argv[3]);
	int startEvalDay = atoi(argv[4]);
    int endEvalDay = atoi(argv[5]);
    int initCorpusStartDay = atoi(argv[6]);
    int initCorpusEndDay = atoi(argv[7]);
    string scoredDir = string(argv[8]);
    string queryFileName = string(argv[9]);
    string year = string(argv[10]);
    string month = string(argv[11]);
    string scenarioBDir = string(argv[12]);
    string parsedDir = string(argv[13]);
	int topN = atoi(argv[14]);
	int topK = atoi(argv[15]);
	string adaptiveThresholdDir = string(argv[16]);
	int runMode = atoi(argv[17]);  // runMode (1: adaptive 2:manual)
	int topNHighRelevant = atoi(argv[18]);
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
	

	int curDay = startEvalDay;
	while (true) {
		// check whether the condition statisfied
		stringstream ss;
		ss << year << "-" << month << "-" << curDay + 1 << "-00";
		struct tm cur_time;
		strptime(ss.str().c_str(),"%Y-%m-%d-%H",&cur_time);
		char cur_suffix[100];
		strftime(cur_suffix,100,"%Y-%m-%d-%H",&cur_time);
		string nextDayParsedFilePath = parsedDir + "statuses.parsed."+string(cur_suffix);
		if (fopen(nextDayParsedFilePath.c_str(),"rb") == NULL) { // current hour parsed file is not ready
			cout << "wait for file " << nextDayParsedFilePath.c_str() << endl;
			sleep(600);
			continue;
		} 

		// while (access(nextParsedFilePath.c_str(),F_OK) != 0) { // current hour scored file is not ready
		// 		cout << "wait for file " << nextDayParsedFilePath.c_str() << endl;
			// sleep(600);
			// continue;
		// } 
		cout << "start generate scenarioB for day" << curDay << endl;
		// next day suffix
		char nextDay_suffix[100];
		strftime(nextDay_suffix, 100, "%Y%m%d", &cur_time);
		// next day threshold file
		string nextDayAdaptiveThresholdFilePath = adaptiveThresholdDir + string(nextDay_suffix) + ".B";
		ofstream nextDayAdaptiveThresholdFile(nextDayAdaptiveThresholdFilePath.c_str());
		//cout << "open nexxt day adatpive file" << endl;	
		cur_time.tm_mday -= 1;  // modify to current day

		// check whether to update the threshold
		char curDay_suffix[100];
		strftime(curDay_suffix,100,"%Y%m%d",&cur_time);
		string curDayAdaptiveThresholdFilePath;
		map<int, string> adaptiveThresholdMap;
		if (runMode == 1 || runMode == 2) {  // adaptive or manual threshold
			if (runMode == 1)
		 		curDayAdaptiveThresholdFilePath = adaptiveThresholdDir + string(curDay_suffix) + ".B";
		 	else 
		 		curDayAdaptiveThresholdFilePath = adaptiveThresholdDir + string(curDay_suffix) + ".M";
		 	while (true) {
			 	if (access(curDayAdaptiveThresholdFilePath.c_str(),F_OK) == 0) {
			 		string line;
			 		ifstream thresholdFile(curDayAdaptiveThresholdFilePath.c_str());
			 		while (getline(thresholdFile, line)) {
			 			vector<string> thresholdVector = Utility::split(line, '\t');
			 			string thresholdInfo = "";
			 			for (int i = 2; i < thresholdVector.size(); i ++) {
			 				if (thresholdInfo == "")
			 					thresholdInfo = thresholdVector[i];
			 				else
			 					thresholdInfo += "\t" + thresholdVector[i];
			 			}
			 			adaptiveThresholdMap.insert(make_pair(atoi(thresholdVector[1].c_str()), thresholdInfo));
			 		}
			 		thresholdFile.close();
			 		break;
			 	}
			 	else {
			 		cout << "Miss adaptive Threshold File: " << curDayAdaptiveThresholdFilePath.c_str() << endl;
			 		if (curDay == startEvalDay) {  // first eval day could have no adaptive threshold
			 			break;
			 		}
			 		else if (curDay == startEvalDay + 1 && runMode == 2) { // manual start from day21
			 			break;
			 		}
			 		else {
			 			cout << "Sleep to wait for adaptive threshold file:" << curDayAdaptiveThresholdFilePath.c_str()<<endl;
			 			sleep(30);   // sleep to wait
			 		}
			 		//cout << "Miss adaptive Threshold File: " << curDayAdaptiveThresholdFilePath.c_str() << endl;
			 	}
		 	}
		}

		//cout <<"size:" << queryVector.size() << endl;
		for (int qid = 0; qid < queryVector.size(); qid ++) {
			//cout << "[" << qid+1 <<"] processed for day" << curDay << endl;
			if (runMode == 1 || runMode == 2) {  // use adaptive threshold for each query
				if (adaptiveThresholdMap.size() > 0) {
					vector<string> thresholdVector = Utility::split(adaptiveThresholdMap[qid+1],'\t');
					relevanceThreshold = atof(thresholdVector[0].c_str());
					highRelevanceThreshold = atof(thresholdVector[1].c_str());
					//novelThreshold = atof(thresholdVector[2].c_str()); // use uniform threshold
				}
				else if (curDay != startEvalDay) {
					cout << "miss adaptive threshold file in day"  << curDay <<endl;
				}
			}
			// record current relevant documents
			vector<Data> currentVector;
			stringstream sqid;
			sqid << qid+1;
			//cout << "curHour:" <<cur_time.tm_hour << endl;
			int curRelevantCount = 0;
			double curRelevanceThreshold = relevanceThreshold;
			while (cur_time.tm_hour < 24) {
				// get scored file of each hour
				strftime(cur_suffix, 100, "%Y-%m-%d-%H", &cur_time);
				string curScoredFilePath = scoredDir + sqid.str() + "/statuses.scored." + cur_suffix;
				ifstream curScoredFile(curScoredFilePath.c_str());
				string line;
				//cout << "[ScenarioB] curscorePath: " << curScoredFilePath.c_str() << endl;
				while (getline(curScoredFile,line)) {
					vector<string> score_info = Utility::split(line, '\t');
					if (atof(score_info[5].c_str()) < curRelevanceThreshold) {
						continue;
					}
					curRelevantCount += 1;
					// update the relevance Threshold
					if (curRelevantCount % 50 == 0) { 
						curRelevanceThreshold += 0.0025;
					}
					if (cur_time.tm_hour % 2 == 0) {
						curRelevanceThreshold -= 0.0025;
					}

					struct Data data;
					data.tid = score_info[0];
					data.timestamp = score_info[1];
					data.content = score_info[2];
					data.dirichletScore = atof(score_info[3].c_str());
					data.JMScore = atof(score_info[4].c_str());
					data.voteScore = atof(score_info[5].c_str());
					currentVector.push_back(data);
				}
				// add one hour
				cur_time.tm_hour += 1;
			}
			// PRF 
			vector<Term> termVector;
            set<string> termSet;
            sort(currentVector.begin(), currentVector.end(), cmp3); // vote score
            // init termVector
            for (int i = 0; i < currentVector.size() && i < topN; i ++) {
                vector<string> terms = Utility::split(currentVector[i].content,' ');
                for (int j = 0; j < terms.size(); j ++) {
                    if (isValidTerm(terms[j]))
                        termSet.insert(terms[j]); 
                }
            }
            //compute termVector
            set<string>::iterator it;
            for (it=termSet.begin(); it!=termSet.end(); it++) {
                struct Term term;
                term.content = *it;
                term.weight = 0;
                for (int j = 0; j < currentVector.size() && j < topN; j ++) {
                    Tweet t(currentVector[j].tid, currentVector[j].timestamp, currentVector[j].content);
                    term.weight += t.get_term_num(term.content) * (currentVector[j].voteScore);
                }
                term.weight *= collection.idf(term.content);
                termVector.push_back(term);
            }
            sort(termVector.begin(),termVector.end(),cmpTerm);
            string new_query = "";
            for (int i = 0; i < termVector.size() && i < topK; i ++) {
                if (new_query == ""){
                    new_query = termVector[i].content;
                }
                else {
                    new_query += " " + termVector[i].content;
                }
            }
            vector<string> queryTerms = Utility::split(queryVector[qid].get_topic(),' ');
            for (int i = 0; i < queryTerms.size(); i ++)
                new_query += " " + queryTerms[i];

            Query pseudoQuery(sqid.str(),new_query);
			// resort current vector
            for (int i = 0; i < currentVector.size(); i ++) {
                Tweet t(currentVector[i].tid, currentVector[i].timestamp,currentVector[i].content);
                double max = 0;
                double min = -20;
                currentVector[i].dirichletScore = (lm.computeKLDivergence(pseudoQuery,t,collection)-min)/(max-min);
                currentVector[i].JMScore = (lm.computeKLDivergenceWithJM(pseudoQuery,t,collection)-min)/(max-min);
                currentVector[i].voteScore = (currentVector[i].dirichletScore+currentVector[i].JMScore)/2;
                //resultFile << currentVector[i].qid << "\t" << currentVector[i].tid << "\t"<< currentVector[i].dirichletScore << "\t"<<currentVector[i].JMScore << "\t" << currentVector[i].ABSScore<<"\t"<<currentVector[i].BM25Score<<"\t"<<currentVector[i].TFIDFScore<<endl;
            }
			// generate novel tweets for each query
			sort(currentVector.begin(), currentVector.end(), cmp3); // vote score
			char res_suffix[100];
			strftime(res_suffix,100,"%Y-%m-%d",&cur_time);
			string resLogPath = scenarioBDir + sqid.str() + "/statuses.logB." + res_suffix;
			string resPath = scenarioBDir + sqid.str()  + "/statuses.scoreB." + res_suffix;
			ofstream resLogFile(resLogPath.c_str());
			ofstream resFile(resPath.c_str());
			// load previous novel files
			vector<Data> novelList = getNovelListForQuery(qid+1,startEvalDay,curDay,year,month,scenarioBDir);
			// generate non-redundant tweets
			cout << "["<<qid+1<<"] has " << currentVector.size() <<" candidateTweets in day" << curDay << endl;
			int curDealedCount = 0;

			// write adaptive file flag
			bool hasWriteAdaptiveThreshold = false;


			int curPushedCount = 0;
			int maxPushedCountOneDay = 10;

			for (int i = 0; i < currentVector.size(); i ++) {
				curDealedCount += 1;
				if (curDealedCount > 100) {   // max dealed count
					break;
				} 
				if (curDealedCount == topNHighRelevant) {  // record the threshold
					//cout << "start write next day adaptive" << endl;	
					nextDayAdaptiveThresholdFile << nextDay_suffix << "\t" << qid + 1
						<< "\t" << relevanceThreshold << "\t" << currentVector[i].voteScore
						<< "\t" << novelThreshold << "\n";
					hasWriteAdaptiveThreshold = true;
				}
				// generate current novel file
				double maxScore = 0;
				string maxTid = currentVector[i].tid;
				for (int j = 0; j < novelList.size(); j ++) {
					Query query1(sqid.str(), currentVector[i].content);
					Query query2(sqid.str(), novelList[j].content);
					Tweet t1(currentVector[i].tid,currentVector[i].timestamp,currentVector[i].content);
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
						//maxIndex = j;
						maxTid = novelList[j].tid;
					}
				}
				if (novelList.size() == 0) {
					if (currentVector[i].voteScore >= highRelevanceThreshold) {
						if (curDay > startEvalDay) { // skip first day 19
							cout << "[" << qid+1 << "]first novel tweet generated" << endl;
							resFile << currentVector[i].tid<<"\t"<<currentVector[i].timestamp
							<<"\t"<<currentVector[i].content<<"\t"<<currentVector[i].dirichletScore
							<<"\t"<<currentVector[i].JMScore<<"\t"<<currentVector[i].voteScore
							<<"\tnovel\t"<<maxTid<<"\t"<<maxScore
							<<"\t"<<relevanceThreshold<<"\t"<<highRelevanceThreshold<<"\t"<<novelThreshold
							<<"\n";
							curPushedCount += 1;
						}
						// struct Data newData(currentVector[i]);
						// novelList.push_back(newData);
					}
					struct Data newData(currentVector[i]);  // aim for current day, do not effect next day
					novelList.push_back(newData);
					// log file
					resLogFile << currentVector[i].tid<<"\t"<<currentVector[i].timestamp
					<<"\t"<<currentVector[i].content<<"\t"<<currentVector[i].dirichletScore
					<<"\t"<<currentVector[i].JMScore<<"\t"<<currentVector[i].voteScore<<"\tnovel"
					<<"\t"<<maxTid<<"\t"<<maxScore
					<<"\t"<<relevanceThreshold<<"\t"<<highRelevanceThreshold<<"\t"<<novelThreshold
					<<"\n";
				}
				else if (maxScore < novelThreshold) { // new novel tweet
					if (currentVector[i].voteScore >= highRelevanceThreshold) {
						if (curDay > startEvalDay && curPushedCount <= maxPushedCountOneDay) { // skip first day 19
							resFile << currentVector[i].tid<<"\t"<<currentVector[i].timestamp
							<<"\t"<<currentVector[i].content<<"\t"<<currentVector[i].dirichletScore
							<<"\t"<<currentVector[i].JMScore<<"\t"<<currentVector[i].voteScore
							<<"\t"<<maxTid<<"\t"<<maxScore
							<<"\t"<<relevanceThreshold<<"\t"<<highRelevanceThreshold<<"\t"<<novelThreshold
							<<"\n";
							curPushedCount += 1;
						}
					}
					struct Data newData(currentVector[i]);  // aim for current day, do not effect next day
					novelList.push_back(newData);
					// log file
					resLogFile << currentVector[i].tid<<"\t"<<currentVector[i].timestamp
					<<"\t"<<currentVector[i].content<<"\t"<<currentVector[i].dirichletScore
					<<"\t"<<currentVector[i].JMScore<<"\t"<<currentVector[i].voteScore<<"\tnovel"
					<<"\t"<<maxTid<<"\t"<<maxScore
					<<"\t"<<relevanceThreshold<<"\t"<<highRelevanceThreshold<<"\t"<<novelThreshold
					<<"\n";
				}
				else {  // not novel
					// log file
					resLogFile << currentVector[i].tid<<"\t"<<currentVector[i].timestamp
					<<"\t"<<currentVector[i].content<<"\t"<<currentVector[i].dirichletScore
					<<"\t"<<currentVector[i].JMScore<<"\t"<<currentVector[i].voteScore<<"\tnotnovel"
					<<"\t"<<maxTid<<"\t"<<maxScore
					<<"\t"<<relevanceThreshold<<"\t"<<highRelevanceThreshold<<"\t"<<novelThreshold
					<<"\n";
				}
			}

			if (!hasWriteAdaptiveThreshold) {  // has not write threshold yet
				// descrease the threshold
				nextDayAdaptiveThresholdFile << nextDay_suffix << "\t" << qid + 1
						<< "\t" << relevanceThreshold - 0.02 << "\t" << highRelevanceThreshold - 0.02
						<< "\t" << novelThreshold << "\n";
			}
			resLogFile.close();
			resFile.close();

			// reset days 
			cur_time.tm_hour = 0;
		}
		// close next day adaptive threshold file
		nextDayAdaptiveThresholdFile.close();
		curDay += 1;
	}
	return 0;
}
