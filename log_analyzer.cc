#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>   

using namespace std;

/*
 * This is the main class
 * It gets a file name and creats a database of all records
 *
 */

/**
 * The main class 
 *
 * This class holds together:
 *   - a URL Database, a set of unique URLs
 *   - a Log Database, a map of date to (url, count)
 *     (url, map) is implemented a map itself
 *     url, is a pointer to the url not the url itself
 *
 * It gets a file name and creats a database of all records.
 */
class LogAnalyzer{
	// Define main data structures
	
	// Hash of set iterator, used in map
	struct set_iterator_hash {
    	size_t operator()(const unordered_set<string>::const_iterator &c_it) const{
        	return hash<string>()(*c_it);
	    }
	};

	typedef unordered_set<string> URL_SET; 		// URL database type
	typedef URL_SET::const_iterator URL_CIT;	// URL database iterator
	typedef unordered_map<URL_CIT, unsigned int, set_iterator_hash> URL_COUNT; 	// URL to count map
	typedef URL_COUNT::iterator COUNT_IT;		// URL to count iterator
	typedef map<time_t, URL_COUNT> LOG_DB; // log file databse data structure, it maps dates to url_counts, 
												// used as ordered map because the output should be sorted and the number of dates are small
	typedef LOG_DB::iterator LOG_DB_IT;			// log_database iterator
	
	// compare function that is used for sort url count data structure
    struct url_count_compare {
        typedef pair<URL_CIT, unsigned int> U_COUT_PAIR;
        bool operator ()(U_COUT_PAIR const& a, U_COUT_PAIR const& b) const {
            if (a.second > b.second){
				return true;
			}else if (a.second == b.second){
				return *(a.first) < *(b.first);
			}else{
				return false;
			}
        }
    };

	/**
	 * Parses lines of log file and extract the date and url
	 *
	 * Param: line of log file
	 * Return: date, url
	 */
	pair<time_t, string>
	pars_line(string &in_line){
		time_t log_date;
		string log_url, date_string;
		pair<time_t, string> result = make_pair<time_t, string> (0, "");
		date_string = in_line.substr(0, in_line.find('|'));
		log_url = in_line.substr(in_line.find('|') + 1);
		try{
			log_date = stol(date_string);
        	log_date = (log_date / sec_in_day) * sec_in_day;
			result = make_pair(log_date, log_url);
    	}catch(...){
			cout << "Error reading line: " << in_line << endl;
		}
    	return result;
	}

	/*
	 * Extract the date from input time
	 * 
	 * Param: unix epoch in seconds
	 * Return: date in GMT
	 */
	string
	get_date(const time_t &raw_date){
		char buffer[16];
		struct tm *gm_date = gmtime(&raw_date);

		strftime(buffer,sizeof(buffer),"%m/%d/%Y GMT",gm_date);
		std::string str_date(buffer);

		return str_date;
	}

	/*
	 * Pointer to unique urls
	 *
	 * If the url does not exists, it adds it to the data base and returns the iterator
	 * It the url exists it returns the iterator to that url
	 * Param: url
	 * Return: iterator of url in url dataset 
	 */
	URL_CIT 
	get_url_ind(std::string url){
    	return url_db.insert(url).first;
	}
	
	/*
	 * Updates the main databse, the log_db
	 *
	 * Checks if there is an entry for the date, if not it adds a new entry with the new url and count 1
	 * Otherwise, there is and entry for that date, so it retrives that entry and updates it:
	 *    If there is an entry for the url it incereases the count
	 *    else create a new map if entry to the count which is 1 and 
	 * 
	 * Param: log date: The entery date
	 *        url: the entry url_iterator
	 */
	void
	update_db(time_t log_date, URL_CIT url_ind){
		LOG_DB_IT l_iterator = log_db.find(log_date);
		if (l_iterator == log_db.end()){ 		// Dont have an entry for that date
			URL_COUNT count;
			count[url_ind] = 1;
			log_db[log_date] = count;
		}else{									// Date exists
			COUNT_IT c_iterator = l_iterator->second.find(url_ind);
			if (c_iterator == l_iterator->second.end()){ 	// First accurance of the url in that date
            	l_iterator->second[url_ind] = 1;
			}else{
				c_iterator->second++; 			// Update count
			}
		}
	}
	const int sec_in_day = 86400;
    string file_name;	// Name of the input file				
    URL_SET url_db;		// URL database, stores unique urls
    LOG_DB log_db;		// Logfile database, stores the whole database

public:
	
	/*
	 * Instructor, Gets the name of the input file and stores it
	 *
	 * Param: input file name
	 */	
    LogAnalyzer(string in_file_name): file_name (in_file_name){};

	/*
	 * Parses the file
	 *
	 * Read teh line one by one and update the database
	 */
	void 
	pars_file(){
		ifstream in_file;
    	in_file.open(file_name.c_str());
	
    	if (not in_file.is_open()){	// Check if it can open the file
        	cout << "Can not open the file " << file_name << endl;
        	return;
    	}

    	string line;

    	while (getline(in_file, line))
    	{
			pair<time_t, string> time_url = pars_line(line);	// Pars line by line to extract date, url
			URL_CIT url_ind = get_url_ind(time_url.second);	// Get the url pointer
			update_db(time_url.first, url_ind);				// update teh database for that date, url
    	}
    	in_file.close();
	}

	/*
	 * Prints the databse as desired
	 *
	 * Goes over database date by date, the map is already sorted by the date key)
	 * Make a copy of date entry to sort it based on the url count
	 */
    void
    print_db(){
        for (auto log_entry: log_db){
            cout << get_date(log_entry.first) << endl;	// Print the date
			vector<pair<URL_CIT, unsigned int> > count_url(log_entry.second.begin(), log_entry.second.end());	// Make a copy to sort
            sort(count_url.begin(), count_url.end(), url_count_compare());
            for (auto t: count_url){
                cout << *(t.first) << ' ' << t.second << endl;	// Print the url count entry
            }
        }
    }
};

int main( int argc, char *argv[] ){
	if ( argc != 2 ){ //check if the file name is there!
    	cout << "No file name! The progrma needs a filename as input\n";
	    cout<<"usage: "<< argv[0] <<" <filename>\n";
    	return -1;
	}

	LogAnalyzer a (argv[1]);	// Creates and instance of the class
	a.pars_file();				// Pars the file
	a.print_db();				// Print it out
	return 0;
}
