#include "IntelWeb.h"
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>

IntelWeb::IntelWeb(){

}

IntelWeb::~IntelWeb(){
	close();
}

bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems){
	close();
	_filePrefix = filePrefix;
	if (inorder.createNew(filePrefix + "_inorder.dat", 2 * maxDataItems) &&		//Create three private DiskMultiMap
		reverse.createNew(filePrefix + "_reverse.dat", 2 * maxDataItems) && 
		count.createNew(filePrefix + "_count.dat", 2 * maxDataItems))
		return true;
	else
		return false;
}

bool IntelWeb::openExisting(const std::string& filePrefix){
	if (_filePrefix == "")
		_filePrefix = filePrefix;
	if (inorder.openExisting(filePrefix + "_inorder.dat") &&					//open three private DiskMultiMap
		reverse.openExisting(filePrefix + "_reverse.dat") &&
		count.openExisting(filePrefix + "_count.dat"))
		return true;
	else{
		close();
		return false;
	}
}

void IntelWeb::close(){
	inorder.close();
	reverse.close();
	count.close();
}

bool IntelWeb::ingest(const std::string& telemetryFile){
	ifstream file(telemetryFile);
	if (!file)
		return false;
	string line;
	if (!openExisting(_filePrefix)){
		cout << "Error opening " << _filePrefix << " in ingest" << endl;
		return false;
	}
	while (getline(file, line)){
		string buf;
		stringstream ss(line);
		vector<string> log;
		while (ss >> buf)
			log.push_back(buf);
		if (!inorder.insert(log[1], log[2], log[0]))						//Store interactions inorder and reverse order
			return false;
		if (!reverse.insert(log[2], log[1], log[0]))
			return false;

		for (int i = 1; i < 3; i++){										//Store the number of apperance
			DiskMultiMap::Iterator it = count.search(log[i]);
			int num = 0;
			if (it.isValid()) {
				MultiMapTuple m = *it;
				num = stoi(m.value);
				count.erase(log[i], m.value, "");
			}
			count.insert(log[i], to_string(num + 1), "");
		}
	}

	close();
	return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators, unsigned int minPrevalenceToBeGood,
	std::vector<std::string>& badEntitiesFound, std::vector<InteractionTuple>& interactions){

	vector<string> _knownbad = indicators;			//current bad entities vector
	unsigned int it_bad = 0;
	set<string> _interactions;						//bad interactions vector
	vector<string> _badEntitiesFound;				//bad entities found
	if (!openExisting(_filePrefix)){
		cout << "Error opening " << _filePrefix << " in crawl" << endl;
		return 0;
	}
	while (it_bad != _knownbad.size()){						//while not empty

		string bad = _knownbad[it_bad];
		DiskMultiMap::Iterator iter[2];
		iter[0] = inorder.search(bad);
		iter[1] = reverse.search(bad);

		if (iter[0].isValid() || iter[1].isValid())		//"bad" is in file
			_badEntitiesFound.push_back(bad);

		for (int i = 0; i < 2; i++){
			if (iter[i].isValid()) {						//find "bad" in inorder
				do {
					MultiMapTuple m = *iter[i];
					string connection = m.value;		//Get the connection file(link) to "bad"
					if (!i)
						_interactions.insert(m.context + " " + m.key + " " + m.value);	//put this interaction to vector
					else
						_interactions.insert(m.context + " " + m.value + " " + m.key);

					DiskMultiMap::Iterator it_c = count.search(connection);		//Get connection's prevalence
					if (it_c.isValid()){
						MultiMapTuple cnt = *it_c;
						if (stoi(cnt.value) <= minPrevalenceToBeGood){
							bool tmp = false;
							for (unsigned int j = 0; j < _knownbad.size(); j++){
								if (_knownbad[j] == connection){
									tmp = true;
									break;
								}
							}
							if (!tmp)
								_knownbad.push_back(connection);
							//_badEntitiesFound.push_back(connection);
						}
					}
					++iter[i];
				} while (iter[i].isValid());
			}
		}
		it_bad++;
	}
	sort(_badEntitiesFound.begin(), _badEntitiesFound.end());				//sort the interations and badEntitiesFound
	badEntitiesFound = _badEntitiesFound;
	set<string>::iterator it = _interactions.begin();
	vector<InteractionTuple> sorted_interactions;
	while (it != _interactions.end()){
		string buf;
		stringstream ss(*it);
		vector<string> log;
		while (ss >> buf)
			log.push_back(buf);
		InteractionTuple tmp;
		tmp.context = log[0];
		tmp.from = log[1];
		tmp.to = log[2];
		sorted_interactions.push_back(tmp);
		it++;
	}
	interactions = sorted_interactions;
	close();
	return badEntitiesFound.size();
}

bool IntelWeb::purge(const std::string& entity){
	if (!openExisting(_filePrefix)){
		cout << "Error opening " << _filePrefix << " in purge" << endl;
		return false;
	}

	//Erase corresponding node in count
	DiskMultiMap::Iterator cnt_itr = count.search(entity);
	MultiMapTuple m = *cnt_itr;
	count.erase(m.key, m.value, m.context);

	struct tobeDeleted
	{
		string first, second;
	};
	tobeDeleted node;
	vector<tobeDeleted> inorder_vec, reverse_vec;

	DiskMultiMap::Iterator inorder_itr = inorder.search(entity);				//Find entity in inorder
	if (inorder_itr.isValid()){
		do{
			MultiMapTuple m = *inorder_itr;
			node.first = m.value;
			node.second = m.context;
			vector<tobeDeleted>::iterator it = inorder_vec.begin();
			bool flag = true;
			while (it != inorder_vec.end()){
				if (it->first == node.first && it->second == node.second){
					flag = false;
					break;
				}
				it++;
			}
			if (flag){
				inorder_vec.push_back(node);
			}
			++inorder_itr;
		} while (inorder_itr.isValid());
	}
	vector<tobeDeleted>::iterator it = inorder_vec.begin();
	while (it != inorder_vec.end()){											//delete entity in inorder and corresponding data in reverse
		inorder.erase(entity, it->first, it->second);
		int tmp = reverse.erase(it->first, entity, it->second);
		DiskMultiMap::Iterator cnt_itr = count.search(it->first);				//Update count 
		if (cnt_itr.isValid()){
			MultiMapTuple m = *cnt_itr, n;
			n = m;
			count.erase(m.key, m.value, m.context);
			if (stoi(n.value) - tmp > 0)
				count.insert(n.key, to_string(stoi(n.value) - tmp), n.context);
		}
		it++;
	}

	DiskMultiMap::Iterator reverse_itr = reverse.search(entity);
	if (reverse_itr.isValid()){
		do{
			MultiMapTuple m = *reverse_itr;
			node.first = m.value;
			node.second = m.context;
			vector<tobeDeleted>::iterator it = reverse_vec.begin();
			bool flag = true;
			while (it != reverse_vec.end()){
				if (it->first == node.first && it->second == node.second){
					flag = false;
					break;
				}
				it++;
			}
			if (flag){
				reverse_vec.push_back(node);
			}
			++reverse_itr;
		} while (reverse_itr.isValid());
	}
	vector<tobeDeleted>::iterator it2 = reverse_vec.begin();
	while (it2 != reverse_vec.end()){
		reverse.erase(entity, it2->first, it2->second);
		int tmp = inorder.erase(it2->first, entity, it2->second);
		DiskMultiMap::Iterator cnt_itr = count.search(it2->first);
		if (cnt_itr.isValid()){
			MultiMapTuple m = *cnt_itr, n;
			n = m;
			count.erase(m.key, m.value, m.context);
			if (stoi(n.value) - tmp > 0)
				count.insert(n.key, to_string(stoi(n.value) - tmp), n.context);
		}
		it2++;
	}


	close();
	return true;
}