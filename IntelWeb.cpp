//
//  IntelWeb.cpp
//  project4
//
//  Created by 文渊 叶 on 3/6/16.
//  Copyright © 2016 wenboye. All rights reserved.
//

#include "IntelWeb.h"
#include "DiskMultiMap.h"
#include "MultiMapTuple.h"
#include "InteractionTuple.h"
#include "BinaryFile.h"
#include <math.h>
#include <algorithm>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>

using namespace std;

IntelWeb::IntelWeb(){
    
}

IntelWeb::~IntelWeb(){
    m_diskmap1.close();
    m_diskmap2.close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems){
     m_diskmap1.close();
     m_diskmap2.close();
     string filename1=filePrefix+"_hash1.dat";
     string filename2=filePrefix+"_hash2.dat";
     if(m_diskmap1.createNew(filename1, floor(0.75*maxDataItems))&&m_diskmap2.createNew(filename2, floor(0.75*maxDataItems)))
         return true;
    m_diskmap1.close();
    m_diskmap2.close();
    return false;
}

bool IntelWeb::openExisting(const std::string& filePrefix){
    m_diskmap1.close();
    m_diskmap2.close();
    string filename1=filePrefix+"_hash1.dat";
    string filename2=filePrefix+"_hash2.dat";
    if(m_diskmap1.openExisting(filename1)&&m_diskmap2.openExisting(filename2))
        return true;
    m_diskmap1.close();
    m_diskmap2.close();
    return false;
}

void IntelWeb::close(){
    m_diskmap1.close();
    m_diskmap2.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile){
    ifstream infile(telemetryFile);
    if(!infile){
        cout<<"cannot open telemetry file!\n";
        return false;
    }
    string line;
    string k,v,c;
   
    while(getline(infile,line)){
        istringstream iss(line);
        if(!(iss>>k>>v>>c)){
            cout<<"Ignoring badly-formatted input line: " << line << endl;
            continue;
        }
        char dummy;
        if (iss >> dummy) // succeeds if there a non-whitespace char
            cout << "Ignoring extra data in line: " << line << endl;
        
        m_diskmap1.insert(v,c,k);
        m_diskmap2.insert(c,v,k);

    }
    return true;
    
}
int IntelWeb::findPrevalence(const std::string& key){
    int prevalence=0;
    DiskMultiMap::Iterator it1 = m_diskmap1.search(key);
    if(it1.isValid()){
        do{
            prevalence++;
            ++it1;
        }while(it1.isValid());
    }
    
    DiskMultiMap::Iterator it2 = m_diskmap2.search(key);
    if(it2.isValid()){
        do{
            prevalence++;
            ++it2;
        }while(it2.isValid());
    }
    return prevalence;

}
void IntelWeb::addToInteractions(std::vector<InteractionTuple>& interactions, const InteractionTuple& I){
    for(int i=0;i<interactions.size();i++){
        if(interactions[i].from==I.from&&interactions[i].to==I.to&&interactions[i].context==I.context){
            return;
        }
    }
    interactions.push_back(I);
}
//void IntelWeb::addToBadEntities(std::vector<std::string>& badEntitiesFound, const std::string &B){
//    for(int i=0;i<badEntitiesFound.size();i++){
//        if(badEntitiesFound[i]==B){
//            return;
//        }
//    }
//    badEntitiesFound.push_back(B);
//}
bool compInteraction(const InteractionTuple& I1, const InteractionTuple& I2){
    if(I1.context<I2.context)
        return true;
    else
      if(I1.context==I2.context){
        if(I1.from<I2.from)
            return true;
        else
            if(I1.from==I2.from){
                if(I1.to<I2.to)
                    return true;
            }
      }
    return false;
}
unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators,
                   unsigned int minPrevalenceToBeGood,
                   std::vector<std::string>& badEntitiesFound,
                   std::vector<InteractionTuple>& interactions
                             ){
    badEntitiesFound.clear();
    interactions.clear();
    InteractionTuple intuple;
    set<string> badEntities;
    unsigned int preBadSize=0;
    
    for(int i=0;i<indicators.size();i++)
    {
        DiskMultiMap::Iterator it1 = m_diskmap1.search(indicators[i]);
        DiskMultiMap::Iterator it2 = m_diskmap2.search(indicators[i]);
        
                if((it1.isValid()||it2.isValid())&&findPrevalence(indicators[i])<minPrevalenceToBeGood){
                    badEntities.insert(indicators[i]);
                }
    }
    
    while(preBadSize!=badEntities.size()){
        preBadSize=badEntities.size();
        set<string>::iterator its=badEntities.begin();
        for(int i=0;i<preBadSize;i++)
        {
            DiskMultiMap::Iterator it1 = m_diskmap1.search(*its);
            if(it1.isValid()){
                do{
                    
                    MultiMapTuple m = *it1;
                    intuple=InteractionTuple(m.key,m.value,m.context);
                    if(findPrevalence(m.value)<minPrevalenceToBeGood){
                        badEntities.insert(m.value);
                    }
                    addToInteractions(interactions,intuple);
                    ++it1;
                    //cout<<*its<<endl;
                    //cout<<intuple.from<<" "<<intuple.to<<" "<<intuple.context<<endl;
                    
                }while(it1.isValid());
                
            }//cout<<"1map1"<<endl;
            
            DiskMultiMap::Iterator it2 = m_diskmap2.search(*its);
            if(it2.isValid()){
                do{
                   
                        MultiMapTuple m = *it2;
                        intuple=InteractionTuple(m.value,m.key,m.context);
                        if(findPrevalence(m.value)<minPrevalenceToBeGood){
                            badEntities.insert(m.value);
                        }
                        addToInteractions(interactions,intuple);
                        ++it2;
                        //cout<<*its<<endl;
                        //cout<<intuple.from<<" "<<intuple.to<<" "<<intuple.context<<endl;
                    
                }while(it2.isValid());
                
            }//cout<<"2map2"<<endl;
             ++its;
        }
    }
    
    for (set<string>::iterator it=badEntities.begin(); it!=badEntities.end(); ++it)
        badEntitiesFound.push_back(*it);
    
    sort(interactions.begin(), interactions.end(), compInteraction);
    return preBadSize;
}

bool IntelWeb::purge(const std::string& entity){
    bool isremove=false;
    DiskMultiMap::Iterator it1 = m_diskmap1.search(entity);
    if(it1.isValid()){
        do{
            
            MultiMapTuple m = *it1;
            if(m_diskmap1.erase(m.key,m.value,m.context)&&m_diskmap2.erase(m.value,m.key,m.context)){
                isremove=true;
                //cout<<"erase "<<m.key<<" "<<m.value<<" "<<m.context<<endl;
            }
            
            ++it1;
        }while(it1.isValid());
        
    }
    DiskMultiMap::Iterator it2 = m_diskmap2.search(entity);
    if(it2.isValid()){
        do{
            
            MultiMapTuple m = *it2;
            if(m_diskmap2.erase(m.key,m.value,m.context)&&m_diskmap1.erase(m.value,m.key,m.context)){
                isremove=true;
                //cout<<"erase "<<m.value<<" "<<m.key<<" "<<m.context<<endl;
            }
            
            ++it2;
        }while(it2.isValid());
        
    }
    return isremove;
}

/*int main(){
    ofstream outfile;
    outfile.open("file.dat");
    outfile << "m1 "<<"a "<<"c " << endl;
    outfile << "m2 "<<"e "<<"a " << endl;
    outfile << "m3 "<<"b "<<"c " << endl;
    outfile << "m4 "<<"g "<<"e " << endl;
    outfile << "m4 "<<"g "<<"f " << endl;
    outfile << "m5 "<<"n "<<"g " << endl;
    outfile << "m5 "<<"b "<<"m " << endl;
    outfile << "m5 "<<"m "<<"n " << endl;
   // outfile << "m5 "<<"m "<<"n " << endl;
    
    outfile.close();
    ifstream infile("file.dat");
    if(!infile){
        cout<<"cannot open telemetry file!\n";
        return false;
    }
    string line;
    string k,v,c;
    
    while(getline(infile,line)){
        istringstream iss;
        iss.str(line);
        if(!(iss>>k>>v>>c)){
            cout<<"Ignoring badly-formatted input line: " << line << endl;
            continue;
        }
        char dummy;
        if (iss >> dummy) // succeeds if there a non-whitespace char
            cout << "Ignoring extra data in line: " << line << endl;
        cout<<k<<" "<<v<<" "<<c<<" "<<endl;
    }
    
    IntelWeb x;
    x.createNew("cyber", 100);
    x.openExisting("cyber");
    x.ingest("file.dat");
    vector<string> indicators;
    indicators.push_back("a");
//    indicators.push_back("disk-eater.exe");
//    indicators.push_back("datakill.exe");
//    indicators.push_back("http://www.stealthyattack.com");
    vector<string> badEntitiesFound;
    vector<InteractionTuple> interactions;
//    cout<<x.crawl(indicators,3,badEntitiesFound,interactions)<<endl;
//    for(int i=0;i<badEntitiesFound.size();i++){
//        cout<<badEntitiesFound[i]<<" ";
//    }
//    cout<<endl;
//    for(int i=0;i<interactions.size();i++){
//        cout<<interactions[i].from<<" "<<interactions[i].to<<" "<<interactions[i].context<<endl;
//    }
    
    cout<<x.purge("m")<<endl;
    cout<<x.purge("m")<<endl;
    cout<<x.purge("n")<<endl;
    
    
    cout<<x.crawl(indicators,4,badEntitiesFound,interactions)<<endl;
    for(int i=0;i<badEntitiesFound.size();i++){
        cout<<badEntitiesFound[i]<<endl;
    }
    cout<<endl;
    for(int i=0;i<interactions.size();i++){
        cout<<interactions[i].from<<" "<<interactions[i].to<<" "<<interactions[i].context<<endl;
    }
}*/