#include "lib.h"
#include <set>
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
    double ABSScore;
    double BM25Score;
    double TFIDFScore;
};

bool isValidTerm(string term) {
    const char *temp = term.c_str();
    if (temp[0] == '@' || temp[0] == '#')
        return false;
    if (term.substr(0,5)=="http:")
      return false;
    for (int i = 0; i < term.length(); i ++) {
        if (!(temp[i]<='9'&&temp[i]>='0'))
          return true;
    }
    return false;
}

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
    return d1.ABSScore > d2.ABSScore;
}

bool cmp4(const Data &d1, const Data &d2) {
    return d1.BM25Score > d2.BM25Score;
}

bool cmp5(const Data &d1, const Data &d2) {
    return d1.TFIDFScore > d2.TFIDFScore;
}
int main(int argc, char **argv) 
{
    if (argc < 8) {
        cout << "argv[1]: originQueryFile" << endl;
        cout << "argv[2]: exQueryFile" << endl;
        cout << "argv[3]: collectionFile" << endl;
        cout << "argv[4]: originFile" << endl;
        cout << "argv[5]: resultFile" << endl;
        cout << "argv[6]: topNDocuments" << endl;
        cout << "argv[7]: topKTerms" << endl;
        return -1;
    }
    int topN = atoi(argv[6]);
    int topK = atoi(argv[7]);
    // read origin Query file
    ifstream queryFile(argv[1]);
    string line;
    vector<string> queryVector;
    while (getline(queryFile, line)) {
       queryVector.push_back(line);  
    }

    // read ex query file
    ifstream exQueryFile(argv[2]);
    vector<string> exQueryVector;
    while (getline(exQueryFile, line)) {
        exQueryVector.push_back(line);
    }

    // read collection
    cout << "start read collection data..." << endl;
    Corpus collection(argv[3]);
    cout << "read collection data done..." << endl;
    // open origin corpus and  result file
    ifstream originFile(argv[4]);
    ofstream resultFile(argv[5]);
    ostringstream resultFileName1, resultFileName2, resultFileName3, resultFileName4, resultFileName5;
    resultFileName1 << argv[5] << "1";
    resultFileName2 << argv[5] << "2";
    resultFileName3 << argv[5] << "3";
    resultFileName4 << argv[5] << "4";
    resultFileName5 << argv[5] << "5";
    ofstream resultFile1(resultFileName1.str().c_str());
    ofstream resultFile2(resultFileName2.str().c_str());
    ofstream resultFile3(resultFileName3.str().c_str());
    ofstream resultFile4(resultFileName4.str().c_str());
    ofstream resultFile5(resultFileName5.str().c_str());
    LanguageModel lm;
    int line_count = 0;
    int current_qid = 111;
    int corpus_size = 0;
    // get corpus file line count
    ifstream readCorpus(argv[3]);
    while (getline(readCorpus, line)) {
        corpus_size ++; 
    }

    vector<Data> currentVector;
    while (getline(originFile, line)) {
        line_count ++;
        if (line_count % 10000 == 0) 
            cout << "score line " << line_count << " done..." << endl;
        vector<string> tabs = Utility::split(line, '\t');
        if (tabs.size() < 3) {
            continue;
        }
        // first stage query expansion
        Query query(tabs[0], queryVector[atoi(tabs[0].c_str())-111]);
        Query exQuery(tabs[0], exQueryVector[atoi(tabs[0].c_str())-111]);
        //lm.expandQuery(query, exQuery, 0.5);
        Tweet tweet(tabs[1], tabs[2]);
        struct Data data;
        data.qid = tabs[0];
        data.tid = tabs[1];
        data.content = tabs[2];
        data.dirichletScore = lm.computeKLDivergence(query,tweet,collection);
        data.JMScore = lm.computeKLDivergenceWithJM(query,tweet,collection);
        data.ABSScore = lm.computeKLDivergenceWithABS(query,tweet,collection);
        data.BM25Score = lm.computeBM25Score(query,tweet,collection);
        data.TFIDFScore = lm.computeTFIDFScore(query, tweet, collection);
        if (atoi(data.qid.c_str()) == current_qid && line_count != corpus_size) {
            currentVector.push_back(data);
        }
        else {
            // write into splitFile
            vector<Term> termVector;
            set<string> termSet;
            sort(currentVector.begin(), currentVector.end(), cmp1);
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
                    Tweet t(currentVector[j].tid, currentVector[j].content);
                    term.weight += t.get_term_num(term.content) * (currentVector[j].dirichletScore+10);
                }
                term.weight *= collection.idf(term.content);
                termVector.push_back(term);
            }
            sort(termVector.begin(),termVector.end(),cmpTerm);
            double topK_weight_sum = 0.0;
            for (int i = 0; i < termVector.size() && i < topK; i ++) {
                topK_weight_sum += termVector[i].weight;
            }
            Query pseudoQuery;
            for (int i = 0; i < termVector.size() && i < topK; i ++) {
                //cout << termVector[i].content << "\t" << termVector[i].weight/topK_weight_sum << endl;
                pseudoQuery.insertTermProb(termVector[i].content, termVector[i].weight/topK_weight_sum);
            }
            // second stage query expansion
            //lm.expandQuery(query, pseudoQuery, 0.6);
            // resort current vector
            for (int i = 0; i < currentVector.size(); i ++) {
                Tweet t(currentVector[i].tid, currentVector[i].content);
                currentVector[i].dirichletScore = lm.computeKLDivergence(query,t,collection);
                currentVector[i].JMScore = lm.computeKLDivergenceWithJM(query,t,collection);
                currentVector[i].ABSScore = lm.computeKLDivergenceWithABS(query,t,collection);
                currentVector[i].BM25Score = lm.computeBM25Score(query,t,collection);
                currentVector[i].TFIDFScore = lm.computeTFIDFScore(query, t, collection);
                resultFile << currentVector[i].qid << "\t" << currentVector[i].tid << "\t"<< currentVector[i].dirichletScore << "\t"<<currentVector[i].JMScore << "\t" << currentVector[i].ABSScore<<"\t"<<currentVector[i].BM25Score<<"\t"<<currentVector[i].TFIDFScore<<endl;
            }
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
            sort(currentVector.begin(), currentVector.end(), cmp1);
            
            currentVector.clear();
            current_qid ++;
            currentVector.push_back(data);
        } 
    }
    return 0;
}

