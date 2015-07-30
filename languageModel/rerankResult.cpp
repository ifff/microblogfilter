#include "lib.h"
#include "Utility.cpp"

using namespace std;

struct Data
{
    string qid;
    string tid;
    string content;
    string dirichletScore;
    string JMScore;
    string ABSScore;
    string BM25Score;
    string TFIDFScore;
};

bool cmp1(const Data &d1, const Data &d2) {
    return atof(d1.dirichletScore.c_str()) > atof(d2.dirichletScore.c_str());
}

bool cmp2(const Data &d1, const Data &d2) {
    return atof(d1.JMScore.c_str()) > atof(d2.JMScore.c_str());
}

bool cmp3(const Data &d1, const Data &d2) {
    return atof(d1.ABSScore.c_str()) > atof(d2.ABSScore.c_str());
}

bool cmp4(const Data &d1, const Data &d2) {
    return atof(d1.BM25Score.c_str()) > atof(d2.BM25Score.c_str());
}

bool cmp5(const Data &d1, const Data &d2) {
    return atof(d1.TFIDFScore.c_str()) > atof(d2.TFIDFScore.c_str());
}

int main(int argc, char **argv) {
    if (argc < 7) {
        cout << "argv[1]: originFile";
        cout << "argv[2]: resultFile1";
        cout << "argv[3]: resultFile2";
        cout << "argv[4]: resultFile3";
        cout << "argv[5]: resultFile4";
        cout << "argv[6]: resultFile5";
        return -1;
    }

    ifstream originFile(argv[1]);
    // open result file 
    ofstream resultFile1(argv[2]);
    ofstream resultFile2(argv[3]);
    ofstream resultFile3(argv[4]);
    ofstream resultFile4(argv[5]);
    ofstream resultFile5(argv[6]);
    string line;
    int current_qid = 111;
    vector<Data> currentVector;
    while (getline(originFile, line)) {
        stringstream ssLine(line);
        struct Data data;
        ssLine >> data.qid;
        ssLine >> data.tid;
        ssLine >> data.content;
        ssLine >> data.dirichletScore;
        ssLine >> data.JMScore;
        ssLine >> data.ABSScore;
        ssLine >> data.BM25Score;
        ssLine >> data.TFIDFScore;
        if (atoi(data.qid.c_str()) == current_qid) {
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
                resultFile4 << currentVector[i].tid<<"\t"<<currentVector[i].content<<endl;
                //resultFile4 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].BM25Score<<"\tmanualLM"<<endl;
            }                                                                                                                 
            sort(currentVector.begin(), currentVector.end(), cmp5);
            for (int i = 0; i < currentVector.size() && i < 1000; i ++) {
                resultFile5 << currentVector[i].qid << "\tQ0\t" << currentVector[i].tid <<"\t"<<i+1<<"\t"<<currentVector[i].TFIDFScore<<"\tmanualLM"<<endl;
            }                                                               
            return 0;
            currentVector.clear();
            current_qid ++;
            currentVector.push_back(data);
        }
    }
    // write last qid result
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
    resultFile1.close();
    resultFile2.close();
    resultFile3.close();
    resultFile4.close();
    resultFile5.close();

}
