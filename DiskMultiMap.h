//
//  DiskMultiMap.hpp
//  project4
//
//  Created by wenboye on 3/6/16.
//  Copyright © 2016 wenboye. All rights reserved.
//

#ifndef DISKMULTIMAP_H_
#define DISKMULTIMAP_H_

#include <string>
#include "MultiMapTuple.h"
#include "BinaryFile.h"
/*You MUST write a class named DiskMultiMap that lets the user:
 1. Efficiently associate a given string key with one or more pairs of string values, where each pair contains a value string and a context string. That means that each association is between a single key, e.g. “foo.exe” and one or more pairs of data, where each pair holds two strings, e.g. {“bar.exe”, “m1234”}, or {“www.yahoo.com”, “m5678”}.
 2. Efficiently look up items by a key string, and receive an iterator to enumerate all matching associations.
 3. Efficiently delete existing associations with a given key.
*/
class DiskMultiMap
{
public:
    
    class Iterator
    {
    public:
        Iterator();
        // You may add additional constructors
        Iterator(bool s,BinaryFile::Offset o,DiskMultiMap* m);
        bool isValid() const;
        Iterator& operator++();
        MultiMapTuple operator*();
        
    private:
        // Your private member declarations will go here
        bool state;
        BinaryFile::Offset m_offset;
        DiskMultiMap* m_diskmap;
    };
    
    DiskMultiMap();
    ~DiskMultiMap();
    bool createNew(const std::string& filename, unsigned int numBuckets);
    bool openExisting(const std::string& filename);
    void close();
    bool insert(const std::string& key, const std::string& value, const std::string& context);
    Iterator search(const std::string& key);
    int erase(const std::string& key, const std::string& value, const std::string& context);
    
private:
    // Your private member declarations will go here
    struct Node{
        Node(){}
        Node(const char k[],const char v[],const char c[]){
            strcpy(key, k);
            strcpy(value,v);
            strcpy(context,c);
        }
        char key[121];
        char value[121];
        char context[121];
        BinaryFile::Offset offnext;
    };
    struct Header{
        Header(){}
        Header(int n):num_buckets(n),deleoffset(0),offset(0){}
        int num_buckets;
        BinaryFile::Offset deleoffset;
        BinaryFile::Offset offset;
    };

    BinaryFile bf;
    BinaryFile::Offset offset;
    Header header;
    std::string filename;
    
};

#endif // DISKMULTIMAP_H_
