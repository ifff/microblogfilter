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


int main(int argc, char **argv) 
{
    if (argc < 4) {
        cout << "argv[1]: queryFile" << endl;
        cout << "argv[2]: collectionFile" << endl;
        cout << "argv[3]: originFile" << endl;
        cout << "argv[4]: scoreFile" << endl;
        return -1;
    }
    //read origin query File
    ifstream originQueryFile("query/origin.query");
    string line;
    vector<string> originQueryVector;
    while (getline(originQueryFile, line)) {
        originQueryVector.push_back(line);
    }
    // read exQuery file
    ifstream exQueryFile(argv[1]);
    //string line;
    vector<string> exQueryVector;
    while (getline(exQueryFile, line)) {
       exQueryVector.push_back(line);  
    }
    // read collection
    cout << "start read collection data..." << endl;
    Corpus collection(argv[2]);
    cout << "read collection data done..." << endl;
    // open origin corpus and result corpus
    ifstream originFile(argv[3]);
    ofstream resultFile(argv[4]);
    ostringstream resultFileName1, resultFileName2, resultFileName3, resultFileName4, resultFileName5;
    resultFileName1 << argv[4] << "1";
    resultFileName2 << argv[4] << "2";
    resultFileName3 << argv[4] << "3";
    resultFileName4 << argv[4] << "4";
    resultFileName5 << argv[4] << "5";
    ofstream resultFile1(resultFileName1.str().c_str());
    ofstream resultFile2(resultFileName2.str().c_str());
    ofstream resultFile3(resultFileName3.str().c_str());
    ofstream resultFile4(resultFileName4.str().c_str());
    ofstream resultFile5(resultFileName5.str().c_str());
    LanguageModel lm;
    int line_count = 0;
    int current_qid = 111;
    int corpus_size = 0;
    // get corpus file size
    ifstream corpusSize(argv[3]);
    while (getline(corpusSize, line)) {
        corpus_size ++;
    }

    vector<Data> currentVector;
    while (getline(originFile, line)) {
        line_count ++;
        if (line_count % 10000 == 0) 
            cout << "score line " << line_count << " done..." << endl;
        vector<string> tabs = Utility::split(line, '\t');
        if (tabs.size() < 3) {
            resultFile << tabs[0] << "\t" << tabs[1] << "\tNULL\tNULL\tNULL\tNULL\tNULL" << endl;
            continue;
        }

        // construct query and expand query
        //Query query(tabs[0], originQueryVector[atoi(tabs[0].c_str())-111]);
        Query query(tabs[0], exQueryVector[atoi(tabs[0].c_str())-111]);
        //lm.expandQuery(query, exQuery, 0.5);
        Tweet tweet(tabs[1], tabs[2]);
        double dirichletScore = lm.computeKLDivergence(query,tweet,collection);
        double JMScore = lm.computeKLDivergenceWithJM(query,tweet,collection);
        double ABSScore = lm.computeKLDivergenceWithABS(query,tweet,collection);
        double BM25Score = lm.computeBM25Score(query,tweet,collection);
        double TFIDFScore = lm.computeTFIDFScore(query, tweet, collection); 
        resultFile << tabs[0] << "\t" << tabs[1] << "\t"<< dirichletScore << "\t" 
             << JMScore << "\t" <<ABSScore<<"\t"<<BM25Score<<"\t" << TFIDFScore << endl;
        struct Data data;
        data.qid = tabs[0];
        data.tid = tabs[1];
        data.content = tabs[2];
        data.dirichletScore = dirichletScore;
        data.JMScore = JMScore;
        data.ABSScore = ABSScore;
        data.BM25Score = BM25Score;
        data.TFIDFScore = TFIDFScore;
        if (atoi(data.qid.c_str()) == current_qid && line_count != corpus_size) {
            currentVector.push_back(data);
        }
        else {
            // write into splitFile
            sort(currentVector.begin(), currentVector.end(), cmp1);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile1 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].dirichletScore<<"\tmanualLM"<<endl;
            }
            sort(currentVector.begin(), currentVector.end(), cmp2);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile2 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].JMScore<<"\tmanualLM"<<endl;
            }                                                                                                                 
            sort(currentVector.begin(), currentVector.end(), cmp3);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile3 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].ABSScore<<"\tmanualLM"<<endl;
            }                                                                                                                 
            sort(currentVector.begin(), currentVector.end(), cmp4);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile4 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].BM25Score<<"\tmanualLM"<<endl;
            }                                                                                                                 
            sort(currentVector.begin(), currentVector.end(), cmp5);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile5 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].TFIDFScore<<"\tmanualLM"<<endl;
            }                                                               
            currentVector.clear();
            current_qid ++;
            currentVector.push_back(data);
        }
    }
    
    resultFile1.close();
    resultFile2.close();
    resultFile3.close();
    resultFile4.close();
    resultFile5.close();   
    originFile.close();
    resultFile.close();
    return 0;
}

