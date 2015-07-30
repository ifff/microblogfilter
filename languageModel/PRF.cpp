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
    if (argc < 7) {
        cout << "argv[1]: queryFile" << endl;
        cout << "argv[2]: collectionFile" << endl;
        cout << "argv[3]: originFile" << endl;
        cout << "argv[4]: newQueryFile" << endl;
        cout << "argv[5]: topNDocuments" << endl;
        cout << "argv[6]: topKTerms" << endl;
        return -1;
    }
    int topN = atoi(argv[5]);
    int topK = atoi(argv[6]);
    // read Query file
    ifstream queryFile(argv[1]);
    string line;
    vector<string> queryVector;
    while (getline(queryFile, line)) {
       queryVector.push_back(line);  
    }
    // read collection
    cout << "start read collection data..." << endl;
    Corpus collection(argv[2]);
    cout << "read collection data done..." << endl;
    // open origin corpus and new query result file
    ifstream originFile(argv[3]);
    ofstream resultFile(argv[4]);
    LanguageModel lm;
    int line_count = 0;
    int current_qid = 111;
    vector<Data> currentVector;
    while (getline(originFile, line)) {
        line_count ++;
        if (line_count % 10000 == 0) 
            cout << "score line " << line_count << " done..." << endl;
        vector<string> data1 = Utility::split(line, '\t');
        if (data1.size() < 3) {
            continue;
        }
        Query query(data1[0], queryVector[atoi(data1[0].c_str())-111]);
        Tweet tweet(data1[1], data1[2]);
        struct Data data;
        data.qid = data1[0];
        data.tid = data1[1];
        data.content = data1[2];
        data.dirichletScore = lm.computeKLDivergence(query,tweet,collection);
        data.JMScore = lm.computeKLDivergenceWithJM(query,tweet,collection);
        data.ABSScore = lm.computeKLDivergenceWithABS(query,tweet,collection);
        data.BM25Score = lm.computeBM25Score(query,tweet,collection);
        data.TFIDFScore = lm.computeTFIDFScore(query, tweet, collection);
        if (atoi(data.qid.c_str()) == current_qid) {
            currentVector.push_back(data);
        }
        else {
            // write into splitFile
            vector<Term> termVector;
            set<string> termSet;
            sort(currentVector.begin(), currentVector.end(), cmp1);
            // init termVector
            topN=10;
            for (int i = 0; i < currentVector.size() && i < topN; i ++) {
                //cout << "tid:" <<currentVector[i].tid<<"\t"<< currentVector[i].content<<endl;
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
            for (int i = 0; i < termVector.size() && i < topK; i ++) {
                resultFile << termVector[i].content.c_str() << " ";
                //cout << termVector[i].content.c_str() << "\t" << termVector[i].weight << endl;
            }
            // add origin query
            vector<string> queryTerms = Utility::split(queryVector[current_qid-111],' ');
            for (int i = 0; i < queryTerms.size(); i ++)
                resultFile << queryTerms[i] << " ";
            resultFile << endl;
            currentVector.clear();
            current_qid ++;
            currentVector.push_back(data);
        } 
    }
    // write last qid result
    vector<Term> termVector;
    set<string> termSet;
    sort(currentVector.begin(), currentVector.end(), cmp1);
    // init termVector
    topN=10;
    for (int i = 0; i < currentVector.size() && i < topN; i ++) {
        //cout << "tid:" <<currentVector[i].tid<<"\t"<< currentVector[i].content<<endl;
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
    for (int i = 0; i < termVector.size() && i < topK; i ++) {
        resultFile << termVector[i].content.c_str() << " ";
    }
    // add origin query
    vector<string> queryTerms = Utility::split(queryVector[59],' ');
    for (int i = 0; i < queryTerms.size(); i ++)
        resultFile << queryTerms[i] << " ";
    originFile.close();
    resultFile.close();
    return 0;
}

