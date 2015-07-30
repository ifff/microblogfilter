#include "lib.h"
#include "Utility.cpp"
#include "Corpus.cpp"
using namespace std;

#ifndef QUERY_CPP
#define QUERY_CPP

class Query
{
    private:
        int word_num;
        string id;
        string topic;
        map<string, double> tf;
        map<string, double> prob;
    public:
        Query()
        {
        }
        Query(string _id, string _topic)
        {
            // Add MicroBlog Track Prefix
            id =  _id;
            topic = _topic;

            vector<string> topic_vector =  Utility::split(_topic,' ');
            word_num = topic_vector.size();
            for (int i = 0; i < topic_vector.size(); i++){
                add_to_dic(topic_vector[i]);
            }
            map<string, double>::iterator it;
            for (it = tf.begin(); it != tf.end(); it ++) {
                prob.insert(make_pair(it->first, it->second/word_num));
            }
        }

        string get_id() 
        {
            return id;
        }

        string get_topic() 
        {
            return topic;
        }

        void insertTermProb(string _term, double _prob)
        {
            tf.insert(make_pair(_term, 1.0));
            prob.insert(make_pair(_term, _prob));
        }

        map<string, double> &get_prob()
        {
            return prob;
        }

        map<string, double> get_tf() 
        {
            return tf;
        }

        double p(string word) 
        {
            /*if (tf.find(word) == tf.end()) {
                return 0.0;
            } else {
                return tf[word] / (double)word_num;
            }*/
            if (prob.find(word) == prob.end()) {
                return 0.0;
            }
            else {
                return prob[word];
            }
        }


        int get_word_num() 
        {
            return word_num;
        }

    private:
        void add_to_dic(string word) 
        {
            if (tf.find(word) == tf.end()) {
                tf.insert(make_pair(word, 1.0));
            } else {
                tf[word] += 1;
            }	
        }
};
#endif
