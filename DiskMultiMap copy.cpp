//
//  DiskMultiMap.cpp
//  project4
//
//  Created by 文渊 叶 on 3/6/16.
//  Copyright © 2016 wenboye. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include "DiskMultiMap.h"
#include "BinaryFile.h"

using namespace std;
//public:
DiskMultiMap::Iterator::Iterator(){
    state=false;
    m_offset=0;
}
// You may add additional constructors
DiskMultiMap::Iterator::Iterator(bool s,BinaryFile::Offset o,DiskMultiMap* m){
    state=s;
    m_offset=o;
    m_diskmap=m;
    
}
bool DiskMultiMap::Iterator::isValid() const{
    return state;
}
DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++(){
    if(!state)
        return *this;
    Node curn,nextn;
    m_diskmap->bf.read(curn, m_offset);
    if(curn.offnext==-1){   //if current node is the last node
        state=false;
        return *this;
    }
    BinaryFile::Offset nextoffset=curn.offnext;
    m_diskmap->bf.read(nextn, nextoffset);
   
    while(strcmp(curn.key,nextn.key)!=0)
    {                                  //if current node is not the last, find next node with the same key
        if(nextn.offnext==-1){
            state=false;
            break;
        }
        nextoffset=nextn.offnext;
        m_diskmap->bf.read(nextn, nextoffset);
    }
    m_offset=nextoffset;
    return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*(){
    MultiMapTuple m;
    if(!state){
        m.key="";
        m.value="";
        m.context="";
    }
    else{
        Node n;
        m_diskmap->bf.read(n, m_offset);
        m.key=n.key;
        m.value=n.value;
        m.context=n.context;
    }
    return m;
}


DiskMultiMap::DiskMultiMap(){
    offset=0;
}
DiskMultiMap::~DiskMultiMap(){
    close();
}
bool DiskMultiMap::createNew(const std::string& filename, unsigned int numBuckets){
    bf.close();
    if (!bf.createNew(filename)){// create a new file
         cout << "Error! Unable to create "<<filename<<"\n";
        return false;
    }
    else
        header=Header(numBuckets);
        bf.write(header, offset);
        offset+=sizeof(Header);
    for(int i=0;i<numBuckets;i++){
        bf.write(0, offset);
        offset+=sizeof(int);
    }
    return true;
}

bool DiskMultiMap::openExisting(const std::string& filename){
    bf.close();
    if (!bf.openExisting(filename)){
        cout << "Error! Unable to find "<<filename<<"\n";
        return false;
    }
    else
        return true;
}
void DiskMultiMap::close(){
    bf.close();
}
bool DiskMultiMap::insert(const std::string& key, const std::string& value, const std::string& context){
    if(key.size()>120||value.size()>120||context.size()>120)
        return false;
    Node n(key.c_str(),value.c_str(),context.c_str());
    n.offnext=-1;
    hash<string> str_hash;
    unsigned long hashValue=str_hash(n.key);
    unsigned int bucket=hashValue%header.num_buckets;
    int noffset;
    BinaryFile::Offset bucketoffset=sizeof(Header)+bucket*sizeof(int);
    bf.read(noffset,bucketoffset);
    BinaryFile::Offset ot=sizeof(Header)+header.num_buckets*sizeof(int);
    BinaryFile::Offset tempoffset=offset;
    while(ot<offset){   //find the empty offset of deleted node
        Node temp;
        bf.read(temp,ot);
        if(!temp.is_active){
            tempoffset=ot;
            break;
        }
        ot+=sizeof(Node);
    }
    if(!noffset) {  //If no node has been inserted into bucket, write the offset that
                     //the node will be inserted at to the bucket
         bf.write(tempoffset, bucketoffset);
    }
    else{    //If node has been inserted into bucket, add the node to the end of linked list
        Node temp;
        do{
           bf.read(temp,noffset);
        if(temp.offnext!=-1)
            noffset=temp.offnext;
        }while(temp.offnext!=-1);
        temp.offnext=tempoffset;
        bf.write(temp, noffset);
    }
    if(bf.write(n, tempoffset)){
        if(tempoffset==offset)
            offset+=sizeof(n);
        return true;
    }
        return false;
    
    
}
DiskMultiMap::Iterator DiskMultiMap::search(const std::string& key){
    hash<string> str_hash;
    unsigned long hashValue=str_hash(key);
    unsigned int bucket=hashValue%header.num_buckets;
    int noffset;
    BinaryFile::Offset bucketoffset=sizeof(Header)+bucket*sizeof(int);
    bf.read(noffset,bucketoffset);
    Node temp;
    BinaryFile::Offset tempoffset=noffset;
    if(!noffset) {  //If no node with the key has been inserted into bucket,return invalid iterator
        DiskMultiMap::Iterator it(false,noffset,this);
        return it;
    }
    else{    //If node has been inserted into bucket,search the node with the key
        bf.read(temp,tempoffset);
        
        while(strcmp(temp.key,key.c_str())!=0){
            if(temp.offnext==-1){
                DiskMultiMap::Iterator it(false,-1,this);
                return it;
            }
            tempoffset=temp.offnext;
            bf.read(temp, tempoffset);
        }
        DiskMultiMap::Iterator it(true,tempoffset,this);
        return it;
        
    }
}
int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context){
    if(key.size()>120||value.size()>120||context.size()>120)
        return false;
    int removedNodes=0;
    hash<string> str_hash;
    unsigned long hashValue=str_hash(key);
    unsigned int bucket=hashValue%header.num_buckets;
    int noffset;
    BinaryFile::Offset bucketoffset=sizeof(Header)+bucket*sizeof(int);
    bf.read(noffset,bucketoffset);
    if(!noffset) {   //If no node with the key has been inserted into bucket, return 0
        return 0;
    }
    else{    //If node has been inserted into bucket, search and erase the node with
             //corresponding key, value, context
        BinaryFile::Offset pos=noffset;
        BinaryFile::Offset next;
        Node curn;
        
        while(pos!=-1){ //remove from the firstNode
                bf.read(curn,pos);
                next=curn.offnext;
                if(!(strcmp(curn.key, key.c_str())==0&&strcmp(curn.value, value.c_str())==0&&strcmp(curn.context, context.c_str())==0))
                    break;
                else{
                    curn.offnext=-1;
                    curn.is_active=false;
                    bf.write(curn, pos); //update the deleted node
                    bf.write(next, bucketoffset); //update the bucket of the deleted node
                    removedNodes++;
                    //cout << "1Successfully remove "<<curn.key<<" "<<curn.value<<"!\n";
                }
                pos=next;
            
        }
        while(pos!=-1){ //if fisrt node is not the one to remove, remove from the next one
                bf.read(curn,pos);
                next=curn.offnext;
                Node nextn;
                if(next==-1)
                    break;
                bf.read(nextn,next);
            
                    if(strcmp(nextn.key, key.c_str())==0&&strcmp(nextn.value, value.c_str())==0&&strcmp(nextn.context, context.c_str())==0)
                    {
                        curn.offnext=nextn.offnext;
                        bf.write(curn, pos); //update the node before deleted node
                        nextn.offnext=-1;
                        nextn.is_active=false;
                        bf.write(nextn, next); //update the deleted node
                        removedNodes++;
                        //cout << "2Successfully remove "<<nextn.key<<" "<<nextn.value<<"!\n";
                    }
                    else
                        pos=next;
            }
    }
    return removedNodes;
}
int main() {
    DiskMultiMap x;
    x.createNew("myhashtable.dat",100); // empty, with 100 buckets
    x.insert("hmm.exe", "pfft.exe", "m52902");
    x.insert("hmm.exe", "pfft.exe", "m52902");
    x.insert("hmm.exe", "pfft.exe", "m10001");
    x.insert("blah.exe", "bletch.exe", "m0003");
    DiskMultiMap::Iterator it = x.search("hmm.exe");
    if (it.isValid())
    {
        cout << "I found at least 1 item with a key of hmm.exe\n";
        do
        {
            MultiMapTuple m = *it; // get the association
            cout << "The key is: " << m.key << endl;
            cout << "The value is: " << m.value << endl;
            cout << "The context is: " << m.context << endl;
            cout << endl;
            ++it; // advance iterator to the next matching item
        } while (it.isValid());
    }
    
    DiskMultiMap::Iterator it1 = x.search("goober.exe");
    if ( ! it1.isValid())
        cout << "I couldn’t find goober.exe\n";
    
    // line 1
    if (x.erase("hmm.exe", "pfft.exe", "m52902") == 2)
        cout << "Just erased 2 items from the table!\n";
    // line 2
    //if (x.erase("hmm.exe", "pfft.exe", "m10001") ==1)
       // cout << "Just erased at least 1 item from the table!\n";
    // line 3
    if (x.erase("blah.exe", "bletch.exe", "m66666") == 0)
        cout << "I didn't erase this item cause it wasn't there\n";
    
    DiskMultiMap::Iterator it2 = x.search("hmm.exe");
    if (it2.isValid())
    {
        cout << "I found at least 1 item with a key of hmm.exe\n";
        do
        {
            MultiMapTuple m = *it2; // get the association
            cout << "The key is: " << m.key << endl;
            cout << "The value is: " << m.value << endl;
            cout << "The context is: " << m.context << endl;
            cout << endl;
            ++it2; // advance iterator to the next matching item
        } while (it2.isValid());
    }
}