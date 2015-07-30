
#include "lib.h" 
#include "Tweet.cpp"
#include "Corpus.cpp"
#include "Query.cpp"

using namespace std;

#ifndef LANGUAGEMODEL_CPP
#define LANGUAGEMODEL_CPP

class LanguageModel
{
    private:
        double mu;              // Diri parameter mu
        double lamda;   // JM parameter
        double delta;   // ABS parameter
    public:
        LanguageModel()
        {
            mu = 100;
            lamda = 0.2;
            delta = 0.7;
        }

        void expandQuery(Query &q1, Query &q2, double rate) {
            map<string, double>::iterator it1,it2;
            for (it1 = q1.get_prob().begin(); it1 != q1.get_prob().end(); it1 ++) {
                it1->second = it1->second * rate;
            }

            for (it2 = q2.get_prob().begin(); it2 != q2.get_prob().end(); it2 ++) {
                it1 = q1.get_prob().find(it2->first);
                if (it1 != q1.get_prob().end()) {
                    it1->second = it1->second + it2->second * (1-rate);
                }
                else {
                    q1.get_prob().insert(make_pair(it2->first, it2->second * (1-rate)));
                }                   
            }
            cout << "after expand query:" << endl;
            for (it1 = q1.get_prob().begin(); it1 != q1.get_prob().end(); it1 ++) {
                cout << it1->first << "\t" << it1->second << endl;
            } 

        }

        double computeTFScore(Query &q, Tweet &t, Corpus &collection) {
            double score = 0.0; 
            map<string, double> tf = q.get_tf();
            map<string, double>::iterator it = tf.begin();
            bool flag = false;
            while (it != tf.end()) {
                string word = it->first;
                double idf_word = collection.idf(word);
                double tf_word = t.get_term_num(word);
                double tfidf = idf_word * tf_word;
                score += tfidf;
                flag = true;
                it ++;
            }
            if(!flag) score = -999999999;
            return score;
        }

        double computeBM25Score(Query &q, Tweet &t, Corpus &collection) {
            double score = 0.0;
            map<string, double> tf = q.get_tf();
            map<string, double>::iterator it = tf.begin();
            bool flag = false;
            while (it != tf.end()) {
                string word = it->first;
                double idf_word = collection.idf(word);
                double f_word = t.get_term_num(word);
                double d_length = t.get_word_num();
                double k1 = 1.2;
                double b = 0.75;
                double avg_length = 4.61337;
                double rate = (k1+1)*f_word / (f_word+k1*(1-b+b*d_length/avg_length));
                score += idf_word * rate;
                flag = true;
                ++it;
            }
            if(!flag) score = -999999999;
            return score;
        }

        double computeKLDivergenceWithJM(Query& q,  Tweet &t, Corpus &collection)
        {
            double score = 0.0;
            map <string, double> tf = q.get_tf();
            map <string, double>::iterator it = tf.begin();
            bool flag = false;
            while(it != tf.end()) {
                string word = it->first;
                double p_w_in_q = q.p(word);
                double p_w_in_t = t.p(word);
                double p_w_in_c = collection.p(word);

                double p_w_in_d = (1-lamda) * p_w_in_t + lamda * p_w_in_c;

                if(p_w_in_d != 0) {
                    score += (p_w_in_q * log(p_w_in_d));
                    flag = true;
                }
                ++it;
            }
            if(!flag) score = -999999999;
            return score;
        } 
        
        double computeKLDivergenceWithABS(Query& q,  Tweet &t, Corpus &collection)
        {
            double score = 0.0;
            map <string, double> tf = q.get_tf();
            map <string, double>::iterator it = tf.begin();
            bool flag = false;
            while(it != tf.end()) {
                string word = it->first;
                double tf_w_in_t = t.get_term_num(word) - delta;
                if (tf_w_in_t < 0) tf_w_in_t = 0;
                double t_length = t.get_word_num();
                double unique_terms_in_t = t.get_unique_term_num();
                double p_w_in_c = collection.p(word);
                double p_w_in_q = q.p(word);
                double p_w_in_d = tf_w_in_t / t_length + delta * unique_terms_in_t *p_w_in_c / t_length; 

                if(p_w_in_d != 0) {
                    score += (p_w_in_q * log(p_w_in_d));
                    flag = true;
                }
                ++it;
            }
            if(!flag) score = -999999999;
            return score;
        }
        
        /**
         * LanguageModel::computeKLDivergence compute the KLDivergence of query q and tweet t
         *
         * @param Query q
         * @param Tweet t
         * @param Corpus ci
         */ 
        double computeKLDivergence(Query& q,  Tweet &t, Corpus &collection)
        {
            double score = 0.0;
            map <string, double> tf = q.get_tf();
            map <string, double>::iterator it = tf.begin();
            bool flag = false;
            while(it != tf.end()) {
                string word = it->first;
                double p_w_in_q = q.p(word);
                double p_w_in_t = t.p(word);
                double p_w_in_c = collection.p(word);

                double alpha = mu / (t.get_word_num() + mu);
                double p_w_in_d = (1-alpha) * p_w_in_t + alpha * p_w_in_c;

                //cout << "word: " << word << "\t" << p_w_in_q << "\t" << p_w_in_t << "\t" << p_w_in_c << "\t" << p_w_in_d<<endl;
                if(p_w_in_d != 0) {
                    score += (p_w_in_q * log(p_w_in_d));
                    flag = true;
                }
                ++it;
            }
            if(!flag) score = -999999999;
            return score;
        }

        

        /**
         * LanguageModel::computeKLDivergence compute the KLDivergence of query q and tweet t using
         * no Smooth Strategy
         *
         * @param Query q
         * @param Tweet t
         * @param Corpus ci
         */ 
        double computeKLDivergenceNoSM(Query& q,  Tweet &t, Corpus &collection)
        {
            double score = 0.0;
            map <string, double> tf = q.get_tf();
            map <string, double>::iterator it = tf.begin();
            bool flag = false;
            while(it != tf.end()) {
                string word = it->first;
                double p_w_in_q = q.p(word);
                double p_w_in_t = t.p(word);
                if(p_w_in_t != 0) {
                    score += 1;
                    flag = true;
                }
                ++it;
            }
            score /= (double)q.get_word_num();
            if(!flag) {
                //nothing found change score to -inf
                score = -99999999;
            }
            return score;
        }
};

#endif
