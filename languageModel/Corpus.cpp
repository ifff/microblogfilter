#ifndef CORPUS_CPP
#define CORPUS_CPP

#include "lib.h"
#include "Utility.cpp"
using namespace std;
class Corpus
{
    private:
        map<string,double> tf;
        int doc_num;
        int term_num;
        map<string, double> df;
    public:
        Corpus(string path) {
            ifstream collectionFile(path.c_str());
            string line;
            doc_num = 243271538;
            term_num = 793037942;
            int line_num = 0;
            while (getline(collectionFile, line)) {
                line_num += 1;
                if (line_num % 10000 == 0) {
                    cout << "read collection " << line_num << " lines..." <<endl;
                }
                vector<string> collection_vector = Utility::split(line, '\t');
                string id = collection_vector[0];
                double termF = atof(collection_vector[2].c_str());
                tf.insert(make_pair(id, termF));
                df.insert(make_pair(id, atof(collection_vector[1].c_str())));
            }            
        }
		
		// read start-end range corpus to init the corpus index information
		Corpus(string parsedDir, string year, string month, int startDay, int endDay) {
			cout << "Initialize corpus from day" << startDay << " to day" << endDay << endl;
			doc_num = 0;
			term_num = 0;
			string base_dir = parsedDir;
			int curDay = startDay;
			stringstream ss;
			ss << year << "-" << month << "-" << curDay << "-00";
			struct tm cur_time;
			strptime(ss.str().c_str(),"%Y-%m-%d-%H",&cur_time);
			char cur_suffix[100];
			strftime(cur_suffix, 100, "%Y-%m-%d-%H",&cur_time);
			while (curDay <= endDay) {			
				string curParsedFilePath = base_dir + "statuses.parsed." + cur_suffix;
				cout << "InitialCorpus: current parsed file path is " << curParsedFilePath.c_str() << endl;
				ifstream curParsedFile(curParsedFilePath.c_str());
				string line;
				while (getline(curParsedFile, line)) {
					vector<string> tweet_vector = Utility::split(line, '\t');
					addNewDocument(tweet_vector[2]);
				}
				curParsedFile.close();
				// update time
				cur_time.tm_hour += 1;
				mktime(&cur_time);
				strftime(cur_suffix, 100, "%Y-%m-%d-%H",&cur_time);
				char new_day[100];
				strftime(new_day, 100, "%d",&cur_time);
				curDay = atoi(new_day);
			}
			writeCorpusInfo();
			cout << "Initialize Corpus completed..." << endl;
		}
		
		// write to file (df and tf)
		void writeCorpusInfo() {
			ofstream dfFile("df.log");
			ofstream tfFile("tf.log");
			map<string, double>::iterator it;
			for (it = df.begin(); it != df.end(); it++) {
				dfFile << it->first << "\t" << it->second << endl;
			}
			for (it = tf.begin(); it != tf.end(); it ++) {
				tfFile << it->first << "\t" << it->second << endl;
			}
			dfFile.close();
			tfFile.close();
		}
		
		/**
		 * update the corpus df and tf when adding new document
		 * @param string tweet
		 */
		void addNewDocument(string tweet_text) {
			vector<string> term_vector = Utility::split(tweet_text, ' ');
			map<string, bool> unique_term;
			doc_num += 1;
			for (int i = 0; i < term_vector.size(); i ++) {
				string word = term_vector[i];
				if (word == "" || word == " ") continue;
				term_num += 1;
				if (tf.find(word) != tf.end()) {
					tf[word] += 1;
				}
				else {
					tf.insert(make_pair(word, 1));
				}
				if (unique_term.find(word) == unique_term.end()) {
					unique_term.insert(make_pair(word, true));
					if (df.find(word) != df.end()) {
						df[word] += 1;
					}
					else {
						df.insert(make_pair(word, 1));
					}
				}
			}
		}


        /**
         * get the tf value from index
         * @param string word
         * @return double the tf value of word
         */ 
        double p(string word) 
        {
            if(tf.find(word) != tf.end()) {
                return (double)tf[word] / (double)term_num;
            } else {
                return 5.0 / (double)term_num;
                //return 0.0;
            }
        }


        double idf(string word) {
            double idf_value = 0.0;
            double df_word;
            if (df.find(word) != df.end()) {
                df_word = df[word];
            }
            else {
                df_word = 5;
            }
            double tmp1 = (double)doc_num - (double)df_word + 0.5;
            double tmp2 = (double)df_word + 0.5;
            idf_value = log(tmp1 / tmp2);
            return idf_value;
        }

};
#endif
