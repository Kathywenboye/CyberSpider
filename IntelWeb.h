//
//  IntelWeb.hpp
//  project4
//
//  Created by wenboye on 3/6/16.
//  Copyright © 2016 wenboye. All rights reserved.
//

#ifndef INTELWEB_H_
#define INTELWEB_H_

#include "InteractionTuple.h"
#include "DiskMultiMap.h"
#include <string>
#include <vector>
/*1. This class MUST be able to “digest” the entire contents of one or more log data files and organize this information into an efficient disk-based data structure that can be “crawled” to discover malicious entities (the discovery process should be an improved version of the one described in the section above).
 2. Given a set of known malicious entities as input, this class MUST be able to “crawl” through your previously built, disk-based data structure to discover the presence of these known malicious entities, as well as associated, unknown malicious entities within the log data. It MUST then return all discovered malicious entities as well as context around all references to the discovered malicious entities (i.e., one or more log entries, {M#, E1, E2}, that detail each discovered malicious entity’s relationship(s) with other entities, malicious or legitimate).*/
class IntelWeb
{
public:
    IntelWeb();
    ~IntelWeb();
    bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
    bool openExisting(const std::string& filePrefix);
    void close();
    bool ingest(const std::string& telemetryFile);
    unsigned int crawl(const std::vector<std::string>& indicators,
                       unsigned int minPrevalenceToBeGood,
                       std::vector<std::string>& badEntitiesFound,
                       std::vector<InteractionTuple>& interactions
                       );
    bool purge(const std::string& entity);
    
private:
    // Your private member declarations will go here
    DiskMultiMap m_diskmap1;
    DiskMultiMap m_diskmap2;
    int findPrevalence(const std::string& key);
    void addToInteractions(std::vector<InteractionTuple>& interactions, const InteractionTuple& I);
};
bool compInteraction(const InteractionTuple& I1, const InteractionTuple& I2);


#endif // INTELWEB_H_

