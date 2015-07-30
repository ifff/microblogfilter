#include "lib.h"
#include "Query.cpp"
#include "LanguageModel.cpp"
#include "Utility.cpp"
#include "Corpus.cpp"
using namespace std;
struct Data
{
    string qid;
    string tid;
    string content;
    double dirichletScore;
    double JMScore;
    double voteScore;  // 0.5 * (dirichletScore + JMScore)
};
bool cmp1(const Data &d1, const Data &d2) {
    return d1.dirichletScore > d2.dirichletScore;
}

bool cmp2(const Data &d1, const Data &d2) {
    return d1.JMScore > d2.JMScore;
}

bool cmp3(const Data &d1, const Data &d2) {
    return d1.voteScore > d2.voteScore;
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


int main(int argc, char **argv) {
	if (argc < 11) {
		cout << "argv[1]: year" << endl;
		cout << "argv[2]: month" << endl;
		cout << "argv[3]: startDay" << endl;
        cout << "argv[4]: initCorpusStartDay" << endl;
        cout << "argv[5]: initCorpusEndDay" << endl; 
        cout << "argv[6]: webqueryFile" << endl;
		cout << "argv[7]: parsedDir" << endl;
        cout << "argv[8]: scoredDir" << endl;
		cout << "argv[9]: originQueryFile" << endl;
		cout << "argv[10]: startHour" << endl;
        return -1;
    }
	string year = string(argv[1]);
    string month = string(argv[2]);

	int startEvalDay = atoi(argv[3]);
    int initCorpusStartDay = atoi(argv[4]);
    int initCorpusEndDay = atoi(argv[5]);
    string queryFileName = string(argv[6]);
	string parsedDir = string(argv[7]);
    string scoredDir = string(argv[8]);
	string originQueryFileName = string(argv[9]);
	int curHour = atoi(argv[10]);
	// init corpus with a date range
	Corpus collection(parsedDir, year, month,initCorpusStartDay,initCorpusEndDay);
	// init query for all interest profile
	ifstream queryFile(queryFileName.c_str());
    string line;
    vector<Query> queryVector;
	int qid = 0;
	// init web query to filster tweets
	//vector< map<string, int> > webQueryTerm;
    while (getline(queryFile, line)) {
		vector<string> query_vector = Utility::split(line, '\t');
		Query query(query_vector[0],query_vector[1]);
        queryVector.push_back(query);
        // init web query terms
       /* vector<string> term_vector = Utility::split(query_vector[1], ' ');
        map<string, int> termMap;
		for (int i = 0; i < term_vector.size(); i ++) {
			if (termMap.find(term_vector[i]) == termMap.end()) {
				termMap.insert(make_pair(term_vector[i],1));
			}
			else {
				termMap[term_vector[i]] += 1;
			}
		}
		webQueryTerm.push_back(termMap);*/
    }
	queryFile.close();
	// init origin query to filter tweets
	ifstream originQueryFile(originQueryFileName.c_str());
	vector< map<string, int> > originQueryTerm;
	while (getline(originQueryFile, line)) {
		vector<string> query_vector = Utility::split(line, '\t');
		vector<string> term_vector = Utility::split(query_vector[1], ' ');
		map<string, int> termMap;
		for (int i = 0; i < term_vector.size(); i ++) {
			if (termMap.find(term_vector[i]) == termMap.end()) {
				termMap.insert(make_pair(term_vector[i],1));
			}
			else {
				termMap[term_vector[i]] += 1;
			}
		}
		originQueryTerm.push_back(termMap);
	}
	originQueryFile.close();
	// init language model
	LanguageModel lm;
	// start day 
	//int curHour = 0;
    ostringstream start_time_stream;

    start_time_stream << year.c_str() << "-" << month.c_str() << "-" << startEvalDay << "-" << curHour;

    //string curScoredFilePath;
    //string nextScoreFilePath;
    char cur_suffix[100];
    char next_suffix[100];
	struct tm cur_time;
	cout <<"start_time:" << start_time_stream.str().c_str() << endl;
	strptime(start_time_stream.str().c_str(),"%Y-%m-%d-%H",&cur_time);
	int curDay = startEvalDay;

	while (true) {
		strftime(cur_suffix, 100, "%Y-%m-%d-%H",&cur_time);
		cout << "cur_suffix: " << cur_suffix <<endl;
        cur_time.tm_hour += 1;
        normTime(cur_time);
		strftime(next_suffix, 100, "%Y-%m-%d-%H",&cur_time);
		cout << "next_suffix: " << next_suffix <<endl;
        cur_time.tm_hour -= 1;
        normTime(cur_time);
		cout << "parsedDir: " << parsedDir.c_str() << endl;
		string curParsedFilePath = parsedDir + "/statuses.parsed." + string(cur_suffix);
		string nextParsedFilePath = parsedDir + "/statuses.parsed."+ string(next_suffix);
		cout << curParsedFilePath.c_str() << endl;
		cout << nextParsedFilePath.c_str() << endl;
		if (fopen(nextParsedFilePath.c_str(),"rb") == NULL) { // current hour parsed file is not ready
			cout << "current parsed file " << curParsedFilePath.c_str() << " is not ready, next file ("
				<< nextParsedFilePath.c_str() << ") does not exist..." << endl;
			sleep(30);
			continue;
		} 
		cout << "curParsedFilePath:" << curParsedFilePath.c_str() << " is ready..."<< endl;
		cout << "nextParsedFilePath:" << nextParsedFilePath.c_str() << " exist..."<< endl;
		string line;
		ifstream curCorpusFile(curParsedFilePath.c_str());
		// get the output path and open output stream
		vector<string> outputScorePath;
		vector<ofstream *> outputScoreStream;
		for (int qid = 0; qid < queryVector.size(); qid ++) {
			stringstream ss;
			ss << qid + 1;
			string curScoredFilePath = scoredDir + ss.str() + "/statuses.scored." + cur_suffix;
			outputScorePath.push_back(curScoredFilePath);
			ofstream *scoreStream = new ofstream();
			scoreStream->open(curScoredFilePath.c_str());
			outputScoreStream.push_back(scoreStream);
		}
		int line_count = 0;
		while (getline(curCorpusFile, line)) {  // calculate the score
			line_count += 1;
			if (line_count % 1000 == 0) {
				cout << curParsedFilePath.c_str() << ": " << line_count << " lines processed..." << endl;
			}
			vector<string> tweet_vector = Utility::split(line, '\t');
			Tweet tweet(tweet_vector[0],tweet_vector[1],tweet_vector[2]);
			
			// update corpus 
			collection.addNewDocument(tweet_vector[2]);
			for (int qid = 0; qid < queryVector.size(); qid ++) {
				// check filter terms
				vector<string> term_vector = Utility::split(tweet_vector[2],' ');
				bool Flag = false;
				for (int i = 0; i < term_vector.size(); i ++) {
					if (originQueryTerm[qid].find(term_vector[i]) != originQueryTerm[qid].end()) {
						Flag = true;
						break;
					}
				}	
				if (!Flag) continue;
				//string query = queryVector[i];
				double dirichletScore = lm.computeKLDivergence(queryVector[qid],tweet,collection);
				double JMScore = lm.computeKLDivergenceWithJM(queryVector[qid],tweet,collection);
				double voteScore = (dirichletScore + JMScore) * 0.5;
				// normalization using sigmoid
				double min = -20.0;
				double max = 0;
				double n_dirichletScore = (dirichletScore-min) / (max - min);
				double n_JMScore = (JMScore-min) / (max-min);
				double n_voteScore = (voteScore-min) / (max-min);
				//cout << "dirichScore:" << dirichletScore << endl;
				*(outputScoreStream[qid]) << tweet_vector[0] << "\t" << tweet_vector[1]
					<< "\t" << tweet_vector[2] << "\t" << n_dirichletScore << "\t" 
					<< n_JMScore << "\t" << n_voteScore << endl;
			}	
		} 
		// update time
		cur_time.tm_hour += 1;
        normTime(cur_time);
/* 		cur_time.tm_hour += 1;
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
		// mktime(&cur_time);
		// mktime(&next_time);

		//reset
		curCorpusFile.close();
		outputScorePath.clear();
		for (int qid = 0; qid < queryVector.size(); qid ++) {
			outputScoreStream[qid]->close();
		}
		outputScoreStream.clear();
	}
	
}
