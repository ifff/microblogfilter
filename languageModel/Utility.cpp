/**
 *
 * The Utility class that contains many useful functions
 *
 */
#include "lib.h"
#include <sstream>
using namespace std;
#ifndef UTILITY_CPP
#define UTILITY_CPP

class Utility
{

  private:
    static bool cmp(const string & a, const string & b)
    {
        return atoi(a.c_str()) < atoi(b.c_str());
    }

  public:
    /**
     * Read the config map from config_file
     * @param char* config_file the config file name
     * @return map<double, string> config 
     */ 
    static map<string,string> load_config_file(string config_file)
    {
        ifstream FH(config_file.c_str());
        // READ THE config FILE AND GET THE PARAMETERS, SAVE TO MAP :CONFIG
        map<string, string> config;
        string line, name, value;
        while (getline(FH, line)){
            int fp = line.find_first_of(':',0);
            name = line.substr(0,fp);
            value = line.substr(fp+2);
            config.insert(make_pair(name,value));		 		
        }
        FH.close();
        return config;
    }


    /**
     * Get file list in a dir
     * @param string dirname
     * @return vector<string> the file list
     */ 
    static vector<string> get_dir_files(string dirname) 
    {
        DIR *dp;
        struct dirent *dirp;
        vector<string> files;
        if((dp=opendir(dirname.c_str()))==NULL){
            perror("[ERROR]can not open queryPath.\n");
            exit(1);
        }
        while((dirp=readdir(dp))!=NULL){
            if((strcmp(dirp->d_name,".")==0)||(strcmp(dirp->d_name,"..")==0)) continue;
            files.push_back(dirp->d_name);
        }
        sort(files.begin(), files.end(), cmp);
        return files;
    }

    /**
     * Get topic string from topic file
     * @param string filename
     * @return string the topic string
     */
    static string get_topic_from_file(string filename) 
    {
        string topic,line;
        ifstream FH(filename.c_str());
        if (!FH) {
            cout << "[ERROR]can not open topic file";
            exit(0);
        }
        //READ THE TOPIC FILE
        while (getline(FH, line)){
            if(line.find("DOC") != string::npos) continue; // SKIP THE FORMAT <DOC>
            topic = topic.append(line);	
            topic += " ";			
        }
        topic.erase(topic.size()-1,1); // remvoe the last space
        FH.close() ;  
        return topic;
    }


    /**
     * split the string to vector
     */ 
    static vector<string> &split(const string &s, char delim, std::vector<std::string> &elems) {
        stringstream ss(s);
        string item;
        while(getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }


    static vector<std::string> split(const std::string &s, char delim) {
        vector<string> elems;
        return split(s, delim, elems);
    }
};

#endif
