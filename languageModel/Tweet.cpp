#include "Corpus.cpp"
#include "Utility.cpp"
using namespace std;

#ifndef TWEET_CPP
#define TWEET_CPP

class Tweet
{
    private:
        int word_num;               //tweet term count
        string id;                  //tweet id
		string timestamp;			// tweet timestamp
        string content;             //the tweet content
        map<string, double> tf;     //tweet tf

    public:
        Tweet(string _id, string _timestamp, string tweet_text)            
        {
            id = _id;
            content = tweet_text;
			timestamp = _timestamp;
            word_num = 0;
            vector<string> word_vector = Utility::split(tweet_text,' ');
            for(int i=0; i<word_vector.size(); i++) {
                add_to_dic(word_vector[i]);
            }
        }

        int get_word_num() 
        {
            return word_num;
        }

        string get_id() 
        {
            return id;
        }

        int get_unique_term_num() {
            return tf.size();
        }

        string get_content() 
        {
            return content;
        }

        double p(string word) 
        {
            if(tf.find(word) != tf.end()) {
                return tf[word] / (double)word_num;                              
            } else {
                return 0.0;
            }
        }

        double get_term_num(string word) {
            if (tf.find(word) != tf.end()) {
                return tf[word];
            }
            else 
              return 0.0;
        }
    private:
        void add_to_dic(string word)
        {
            if(word.length() == 0) return;
            word_num ++;
            if (tf.find(word) != tf.end()) {
                tf[word] += 1;
            } else {
                tf.insert(make_pair(word, 1));
            }
        }
};
#endif
