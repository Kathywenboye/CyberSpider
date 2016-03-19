//
//  DiskList.cpp
//  project4
//
//  Created by wenboye on 3/5/16.
//  Copyright © 2016 wenboye. All rights reserved.
//

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include "BinaryFile.h"
using namespace std;

struct DiskNode {
    DiskNode(){}
    DiskNode(const char* v, BinaryFile::Offset n,BinaryFile::Offset p) :  next(n),pos(p) {
        value=new char[strlen(v)+1];
        strcpy(value, v);
    }
    char* value;
    BinaryFile::Offset next;// offset of next node
    BinaryFile::Offset pos; // current offset
};

class DiskList
{
public:
    DiskList(const std::string& filename){
        BinaryFile bf;
        if (!bf.createNew(filename))// create a new file
            cout << "Error! Unable to create "<<filename<<"\n";
        else{
            cout << "Successfully created file "<<filename<<"\n";
            BinaryFile::Offset offsetOfFirstNode = 4; // like a head pointer
            bf.write(offsetOfFirstNode, 0);
            this->filename=filename;
            firstNode=0;
            emptySlot=4;
            numOfNodes=0;
            deletedNodes=0;
        }
    }
    
    bool push_front(const char* data){
        if(strlen(data)>=256)
            return false;
        BinaryFile bf;
        if (!bf.openExisting(filename))
            cout << "Error! Unable to find "<<filename<<"\n";
        else
        {
            BinaryFile::Offset next= firstNode;
            
            if(!deletedNodes) //if there is no deleted nodes
            {
                DiskNode dn=DiskNode(data, next, emptySlot);
                if (!bf.write(dn,emptySlot))
                    cout << "Error writing "<<data<<" to file!\n";
                else
                {
                    cout << "Successfully wrote "<<data<<" to offset "<<emptySlot<<"!\n";
                    //int n=sizeof(dn);
                    firstNode=emptySlot;
                    emptySlot=emptySlot+sizeof(DiskNode);
                    numOfNodes++;
                    return true;
                }
            }else
            {
                
                for(int i=4;i<emptySlot;){
                    DiskNode predn;
                    if(bf.read(predn,i))
                    {
                        if(predn.next==-1){
                            DiskNode dn=DiskNode(data, next, predn.pos);
                            if (!bf.write(dn,predn.pos))
                                cout << "Error writing "<<data<<" to file!\n";
                            else
                            {
                                cout << "Successfully wrote "<<data<<" to offset "<<predn.pos<<"!\n";
                                firstNode=predn.pos;
                                deletedNodes--;
                                numOfNodes++;
                                return true;
                            }
                        }
                    }
                    i=i+sizeof(DiskNode);
                }
            }
        }
        return false;
    }
    bool remove(const char* data){
        BinaryFile bf;
        int cnt=numOfNodes;
        if (!bf.openExisting(filename))
                cout << "Error! Unable to find "<<filename<<"\n";
        else
            {
                BinaryFile::Offset pos=firstNode;
                BinaryFile::Offset next;
                DiskNode curdn;

                for(int i=0;i<numOfNodes;i++){ //remove from the beginning to set down firstNode
                    
                    if(bf.read(curdn,pos))
                    {
                        next=curdn.next;
                        if(strcmp(curdn.value, data)!=0)
                            break;
                        else{
                            firstNode=curdn.next;
                            curdn.next=-1;
                            bf.write(curdn, pos);
                            deletedNodes++;
                            cnt--;
                            cout << "1Successfully remove "<<curdn.value<<"!\n";
                        }
                        pos=next;
                    }
                }
                for(int i=0;i<numOfNodes-1;i++){ //fisrt node is not the one to remove, remove from the next node
                    if(bf.read(curdn,pos))
                    {
                        next=curdn.next;
                        DiskNode nextdn;
                        if(!next)
                            break;
                        if(bf.read(nextdn,next)){
                           if(strcmp(nextdn.value, data)==0){
                             curdn.next=nextdn.next;
                             bf.write(curdn, pos);
                             nextdn.next=-1;
                             bf.write(nextdn, next);
                             deletedNodes++;
                             cnt--;
                             cout << "2Successfully remove "<<nextdn.value<<"!\n";
                             }
                           else
                            pos=next;
                        }
                    }
                           
                }
            }
        
        if(cnt<numOfNodes){
            numOfNodes=cnt;
            return true;
        }
        else
            return false;
    }
    void printAll(){
        BinaryFile bf;
        if (!bf.openExisting(filename))
            cout << "Error! Unable to find "<<filename<<"\n";
        else
        {
            BinaryFile::Offset pos=firstNode;
            for(int i=0;i<numOfNodes;i++){
              DiskNode dn;
              if(!bf.read(dn,pos))
                cout << "Error! Unable to read the"<<i+1<<" node!\n";
              else{
                  cout<<dn.value<<endl;
                  pos=dn.next;
              }
           }
        }
    }
private:
    
    string filename;
    BinaryFile::Offset firstNode;
    BinaryFile::Offset emptySlot;
    int numOfNodes;
    int deletedNodes;
    
};
int main()
{
    DiskList x("mylist.dat");
    x.push_front("Fred");
    x.push_front("Lucy");
    x.push_front("Ethel");
    x.push_front("Ethel");
    x.push_front("Lucy");
    x.push_front("Fred");
    x.push_front("Ethel");
    x.push_front("Ricky");
    x.push_front("Lucy");
    x.remove("Lucy");
    x.push_front("Fred");
    x.push_front("Ricky");
    x.printAll();  // writes, one per line
		  // Ricky  Fred  Ricky  Ethel  Fred  Ethel  Ethel  Fred
}
