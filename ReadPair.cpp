/*
 * ReadPair.cpp
 *
 * This class is meant to act as a central read object so that processing through the multiple processors
 * is simple. The class type requires the bases and quality score strings from both halves of the read
 * I figure most of the string operations can be centralized here, reducing text in the main class
 * where we are splitting the files, filling the hashmap, and then working with each pair.
 *
 */
#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>
#include <vector>
#include "ReadPair.hpp"
using namespace std;

// Partial Constructor for the ReadPair class. Used on the first read.
ReadPair::ReadPair(string d1, string r1, string q1, int threadNum) {
	ID1 = d1;
	read1 = r1;
	qual1 = q1;
	fRead = "";
	fQual = "";
	lAdap = 0;
	lAdapLength = 0;
	rAdap = 0;
	rAdapLength = 0;
	merged = 0;
	tRem = 0;
	badRead = 0;
	badSide = 0;
	tNum = threadNum;
	vector<string> PrintLongestArray;
}

// Constructor for the ReadPair class. Takes the bases and quality of both sides of a read
ReadPair::ReadPair(string d1, string d2, string r1, string q1,string r2, string q2, int threadNum) {
	ID1 = d1;
	ID2 = d2;
	read1 = r1;
	read2 = r2;
	qual1 = q1;
	qual2 = q2;
	fRead = "";
	fQual = "";
	lAdap = 0;
	lAdapLength = 0;
	rAdap = 0;
	rAdapLength = 0;
	merged = 0;
	tRem = 0;
	badRead = 0;
	badSide = 0;
	tNum = threadNum;
	vector<string> PrintLongestArray;
	//Compile();
}

// Adds the data for read 2 and then compiles.
void ReadPair::addR2(string d2, string r2, string q2) {
	ID2 = d2;
	read2 = r2;
	qual2 = q2;
	//Compile(); //call after
}

// Quality test for the two parts of this read. Since we aren't yet sure what will be done after finding the
// bad pairs, and because many of the reads look to be failing, I currently just have a failure/pass print
int ReadPair::qualPass() {
	if(fRead.compare("") == 0) {
		//test first half
		int ret = 0;
		int qual = -1;
		int failCtr = 0;
		for(string::iterator c = qual1.begin(); c != qual1.end(); ++c) {
			qual = int(*c)-33;
			//if(int(*c)-33 < 15) {
			if(qual < 15) failCtr+=1;
		}
		if (failCtr < 10) ret += 1;

		//test second half
		qual = -1;
		failCtr = 0;
		for(string::iterator c = qual2.begin(); c != qual2.end(); ++c) {
			qual = int(*c)-33;
			if(qual < 15) failCtr+=1;
		}
		
		if (failCtr < 10) ret += 2;
		
		if(ret == 0) {
			badSide = 3;
			badRead=2;
			
		} else if(ret == 1) {
			fRead = read1;
			fQual = qual1;
			badSide = 2;
			badRead = 1;
			
		} else if (ret == 2) {
			fRead = read2;
			fQual = qual2;
			badSide = 1;
			badRead = 1;
		}
		return ret;
		
	} else { // if reads were combined
		int qual = -1;
		int failCtr = 0;
		for(string::iterator c = fQual.begin(); c != fQual.end(); ++c) {
			qual = int(*c)-33;
			if(qual < 15) failCtr+=1;
		}
		
		if (failCtr < 10) return 3;
		else {
			badRead = 2;
			return 0;
		}
	}
}

// This method strips the leading T and trailing 2 T's by changing their quality scores to #(2)
void ReadPair::tStrip() {
    if(read1.at(0) == 'T') {
        qual1[0] = '#';
        tRem = 1;
    }
    if((read1.at(read1.length() - 1) == 'T') && (read1.at(read1.length() - 2) == 'T')) {
        qual1[qual1.length() - 1] = '#';
        qual1[qual1.length() - 2] = '#';
        tRem = 1;
    }
    if(read2.at(0) == 'T') {
        qual2[0] = '#';
        tRem = 1;
    }
    if((read2.at(read2.length() - 1) == 'T') && (read2.at(read2.length() - 2) == 'T')) {
        qual2[qual2.length() - 1] = '#';
        qual2[qual2.length() - 2] = '#';
        tRem = 1;
    }
}

// Look for 11 overlapping nucleotides. Returns max overlap over 10
int ReadPair::oCheck() {
	int i1 = 0; //looks at read1
	int i2 = 0; //looks at read2
	//longestOLap will keep the largest number of overlapping base matches found. Since there could
	//POTENTIALLY be a section of overlap past the first instance of a match, we will test the whole read
	//just to be certain we found the full overlap

	//int longestOLap = 0;

	//i1Temp will act as temporary i1 to iterate up the string, searching for continual matches (overlap)
	//oCtr will be an overlap counter, counting up for each continual match.
	int i1Temp, oCtr;
	while (i1 < (int)read1.length() - 10) {
		if (read1[i1] == read2[i2]) {
			i1Temp = i1;
			while (i1Temp < (int)read1.length()  && read1[i1Temp] == read2[i2]) {
				++oCtr;
				if (oCtr > 10 && i1Temp == (int)read1.length() - 1) {
					i2 += 1;
					fRead = read1.substr(0,i1) + read2;
					fQual = qual1.substr(0,i1) + qual2;
					merged = 1;
					return 1;
				}
				++i1Temp;
				++i2;
			}
			oCtr = 0;
			i2 = 0;
		}
		++i1;
	}
	return 0;
}

int ReadPair::aRemove() {
    const int MinLengthAdapters = 6; // Min Length for accepted Adapters
    const int NumberOfAdapters = 27; // Amount of Adapters we are using // I use this instead of length, for more flexibility

    // These are for Return
    // Returns 1 if Only removed from First read
    // 2 if Only removed from Second read
    // 3 if removed from both
    // 0 if none removed
    int AdapterRemoved = 0;
    bool AdapterFound = false;
    bool AdapterFound2 = false;

    string PrintLongestAdapter;
    string PrintLongestAdapter2;

	string Adapters[] =
	{
		"",
	    "AATGATACGGCGACCACCGAGATCTACACTCTTTCCCTACACGACGCTCTTCCGATCT",
	    "CAAGCAGAAGACGGCATACGAGATCGGTCTCGGCATTCCTGCTGAACCGCTCTTCCGATCT",
	    "AATGATACGGCGACCACCGAGATCTACACTCTTTCCCTACACGACGCTCTTCCGATCT",
	    "AGATCGGAAGAGCGTCGTGTAGGGAAAGAGTGTAGATCTCGGTGGTCGCCGTATCATT",
	    "CAAGCAGAAGACGGCATACGAGATCGGTCTCGGCATTCCTGCTGAACCGCTCTTCCGATCT",
	    "AGATCGGAAGAGCGGTTCAGCAGGAATGCCGAGACCGATCTCGTATGCCGTCTTCTGCTTG",
	    "TTTTTTTTTTAATGATACGGCGACCACCGAGATCTACAC",
	    "TTTTTTTTTTCAAGCAGAAGACGGCATACGA",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACATCACGATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACCGATGTATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACTTAGGCATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACTGACCAATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACACAGTGATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGCCAATATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACCAGATCATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACACTTGAATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGATCAGATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACTAGCTTATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGGCTACATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACCTTGTAATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACAGTCAACAATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACAGTTCCGTATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACATGTCAGAATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACCCGTCCCGATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGTCCGCACATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGTGAAACGATCTCGTATGCCGTCTTCTGCTTG",
	    "GATCGGAAGAGCACACGTCTGAACTCCAGTCACGTGGCCTTATCTCGTATGCCGTCTTCTGCTTG"
	};
	// If fRead(Overlap) occured
	
	//Commenting out this entire Overlap / No Overlap part for testing
	/*
    if (!fRead.empty()) {
		string str = fRead;
		// (this is the String from Sequence)
		
		string LongestAdapter = ""; // Longest Adapter Found. - This is what SHOULD be Removed
		int counter = 0; // Counter for Adapters
		int RemoveAdapter = 0; // Longest Adapter that is found in sequence (The number in my array) ETC Adapter Number 4
		vector<int> NumbOfArrays; //Array for all the adapters found - For testing
		AdapterFound = false; // If Adapter Found
		
		while (counter < NumberOfAdapters) { // While loop to get every Adapter.
			string AdapterSearch = ""; // Assigning string AdapterSearch to the current Adapter we are searching through
			AdapterSearch.assign(Adapters[counter]);
			int endIndex = AdapterSearch.length(); // Again not needed, could use AdapterSearch.length() in the for loop

			string FoundString = "";
			// For loop, From the first Character, to the length of the Adapter - Minimum characters matching (6) in our case
			// (We don't want to look for matching 1/2/3/4/5 adapter matches, since these couldve matched randomly.)
			// (C T G A The chances of one of our etc 30 adapters to have a matching any 2 character start is 2/30, but 6 matching randomly is 0.007/30)
			// Reducing the chance for random matches
			for (int startIndex = 0; startIndex <= (endIndex - MinLengthAdapters); startIndex = startIndex + 1) {
				// Creating Temp Strings for match - Printing
				string CurrSeq = str.substr(0, (endIndex - startIndex));
				string CurrAdap = AdapterSearch.substr(startIndex, (endIndex)); // Looking at LAST part of the sequence

				//Adapter Found
				size_t found = CurrSeq.find(CurrAdap);
				if (found != string::npos) {
					FoundString.assign(CurrSeq);
					NumbOfArrays.push_back(counter);

					//If First Adapter Found
					if (AdapterFound == false) {
						LongestAdapter.assign(FoundString);
						AdapterFound = true;
						RemoveAdapter = counter; // assigning adapter to remove
						break;
					} else { // If another adapter is found
						if(FoundString.length() >= LongestAdapter.length()) { // If longer than previous (Currently >= !!)
							LongestAdapter.assign(FoundString);
							RemoveAdapter = counter;
							break;
						} else break;
					} // For the else
				} // If Found
			} // For Loop
			counter ++;
		}
		
		// Basically the same as first Half, the only thing different is the For loop
		// Since it is now checking the last part of the sequence, going in a different direction

		counter = 0; // Counter for Adapters
		string LongestAdapter2 = ""; // Longest Adapter Found. - This is what SHOULD be Removed
		int RemoveAdapter2 = 0; // Longest Adapter that is found in sequence (The number in my array)
		vector<int> NumbOfArrays2; //Array for all the adapters found - For testing
		AdapterFound2 = false; // If Adapter Found

		// str.length to avoid too small sequences
		while (counter < NumberOfAdapters) { //
			string AdapterSearch = "";
			AdapterSearch.assign(Adapters[counter]);
			int endIndex = AdapterSearch.length();
			string FoundString = "";
			int CurrentIncrementer = 0; // Incrementer for the substring Current Adapter
			// from last character minus the length of the adapter. to the last character minus MinLengthAdapters

			for (int startIndex = ((int)str.length()- endIndex); startIndex <= ((int)str.length()-MinLengthAdapters); startIndex = startIndex + 1) {
				string CurrSeq = str.substr(startIndex, (startIndex - endIndex));
				string CurrAdap = AdapterSearch.substr(0, (endIndex - CurrentIncrementer)); //Looking at FIRST part of Sequence

				std::size_t found = CurrSeq.find(CurrAdap);
				if (found != string::npos) {
					FoundString.assign(CurrSeq);
					NumbOfArrays2.push_back(counter);

					if (AdapterFound2 == false) { // If first Adapter Found
						LongestAdapter2.assign(FoundString);
						AdapterFound2 = true;
						RemoveAdapter2 = counter; // assigning adapter to remove
						break;
					} else { // If another adapter is found
						if(FoundString.length() >= LongestAdapter2.length()) {
							LongestAdapter2.assign(FoundString);
							RemoveAdapter2 = counter;
							break;
						} else break;
					}
				}
				CurrentIncrementer++;
			}
			counter++;
		}

		int FirstLength = LongestAdapter.length();
		int SecondLength = LongestAdapter2.length();

		PrintLongestAdapter.assign(Adapters[RemoveAdapter]);
		PrintLongestAdapter2.assign(Adapters[RemoveAdapter2]);
		
		lAdap = RemoveAdapter;
		lAdapLength = FirstLength;
		rAdap = RemoveAdapter2;
		rAdapLength = SecondLength;

		//For testing purpose, not replacing the original string, just creating a new
		fRead = str.substr(FirstLength, (str.length() - FirstLength - SecondLength));
		
	
		
	} else {
		
		// Comment out all of this before <-
		
	*/	
		//Changed this to Read2, to read from opposite side of what we have done already
		
        string str1 = read2;
		// (this is the String from Sequence)

		string LongestAdapter = ""; // Longest Adapter Found. - This is what SHOULD be Removed
		int counter = 0; // Counter for Adapters
		int RemoveAdapter = 0; // Longest Adapter that is found in sequence (The number in my array) ETC Adapter Number 4
		vector<int> NumbOfArrays; //Array for all the adapters found - For testing
		AdapterFound = false; // If Adapter Found

		// str.length to avoid too small sequences
		while (counter < NumberOfAdapters){ // While loop to get every Adapter.
			string AdapterSearch = ""; // Assigning string AdapterSearch to the current Adapter we are searching through
			AdapterSearch.assign(Adapters[counter]);
			int endIndex = AdapterSearch.length(); // Again not needed, could use AdapterSearch.length() in the for loop
			string FoundString = "";

			// For loop, From the first Character, to the length of the Adapter - Minimum characters matching (6) in our case
			// (We don't want to look for matching 1/2/3/4/5 adapter matches, since these couldve matched randomly.)
			// (C T G A The chances of one of our etc 30 adapters to have a matching any 2 character start is 2/30, but 6 matching randomly is 0.007/30)
			// Reducing the chance for random matches
			for (int startIndex = 0; startIndex <= (endIndex - MinLengthAdapters); startIndex = startIndex + 1) {
				// Creating Temp Strings for match - Printing
				string CurrSeq = str1.substr(0, (endIndex - startIndex));
				string CurrAdap = AdapterSearch.substr(startIndex, (endIndex)); // Looking at LAST part of the sequence

				//Adapter Found
				size_t found = CurrSeq.find(CurrAdap);
				if (found != string::npos) {
					FoundString.assign(CurrSeq);
					NumbOfArrays.push_back(counter);

					//If First Adapter Found
					if (AdapterFound == false) {
						LongestAdapter.assign(FoundString);
						AdapterFound = true;
						RemoveAdapter = counter; // assigning adapter to remove
						break;
					} else { // If another adapter is found
						if(FoundString.length() >= LongestAdapter.length()) { // If longer than previous (Currently >= !!)
							LongestAdapter.assign(FoundString);
							RemoveAdapter = counter;
							break;
						} else break;
					}
				}
			}
			counter ++;
		}

		//Basically the same as first Half, the only thing different is the For loop
		// Since it is now checking the last part of the sequence, going in a different direction

		//Second
		
		//Changed this to Read1 (To read from opposite side of what we have done already)
		string str2 = read1;
		counter = 0; // Counter for Adapters
		string LongestAdapter2 = ""; // Longest Adapter Found. - This is what SHOULD be Removed
		int RemoveAdapter2 = 0; // Longest Adapter that is found in sequence (The number in my array)
		std::vector< int > NumbOfArrays2; //Array for all the adapters found - For testing
		AdapterFound2 = false; // If Adapter Found
		// str.length to avoid too small sequences
		while (counter < NumberOfAdapters) {
			string AdapterSearch = "";
			AdapterSearch.assign(Adapters[counter]);
			int endIndex = AdapterSearch.length();
			string FoundString = "";
			int CurrentIncrementer = 0; // Incrementer for the substring Current Adapter

			// from last character minus the length of the adapter. to the last character minus MinLengthAdapters
			for (int startIndex = ((int)str2.length()- endIndex); startIndex <= ((int)str2.length()-MinLengthAdapters); startIndex = startIndex + 1) {
				string CurrSeq = str2.substr(startIndex, (startIndex - endIndex));
				string CurrAdap = AdapterSearch.substr(0, (endIndex - CurrentIncrementer)); //Looking at FIRST part of Sequence
				size_t found = CurrSeq.find(CurrAdap);
				
				if (found != string::npos) {
					FoundString.assign(CurrSeq);
					NumbOfArrays2.push_back(counter);
					if (AdapterFound2 == false) { // If first Adapter Found
						LongestAdapter2.assign(FoundString);
						AdapterFound2 = true;
						RemoveAdapter2 = counter; // assigning adapter to remove
						break;
					} else { // If another adapter is found
						if(FoundString.length() >= LongestAdapter2.length()) {
							LongestAdapter2.assign(FoundString);
							RemoveAdapter2 = counter;
							break;
						} else break;
					}
				}
				CurrentIncrementer++;
			}
			counter ++;
		}

		// Adapter Removal
		int FirstLength = LongestAdapter.length();
		int SecondLength = LongestAdapter2.length();
		PrintLongestAdapter = Adapters[RemoveAdapter];
		PrintLongestAdapter2 = Adapters[RemoveAdapter2];
    		
		lAdap = RemoveAdapter;
		lAdapLength = FirstLength;
		rAdap = RemoveAdapter2;
		rAdapLength = SecondLength;

		//Changed sides to look for in both read 1 and read 2. Now looking at opposite ends

		read2 = str1.substr(FirstLength, (str1.length() - FirstLength));
		read1 = str2.substr(0, (str2.length() - SecondLength));
    //}

	if ((AdapterFound == true) && (AdapterFound2 == true)) {
		PrintLongestArray.push_back(PrintLongestAdapter);
		PrintLongestArray.push_back(PrintLongestAdapter2);
		AdapterRemoved = 3;
		
	} else if ((AdapterFound == true) && (AdapterFound2 == false)) {
		PrintLongestArray.push_back(PrintLongestAdapter);
		AdapterRemoved = 1;
	
	} else if ((AdapterFound == false) && (AdapterFound2 == true)){
		PrintLongestArray.push_back(PrintLongestAdapter2);
		AdapterRemoved = 2;
	
	} else {
		AdapterRemoved = 0;
	}

    // Returns 1 if only First Half Removed
    // Returns 2 if only Second Half Removed
    // Returns 3 if both Removed
	return AdapterRemoved;
}

void ReadPair::passOutFile() {
	if(fRead.compare("") == 0) {
		/*char fileName1[15];  //STOP USING SPRINTF. THIS IS WHAT NOT TO DO.
		sprintf(fileName1, "./results/read1Pass_%i.fastq", tNum); 
		ofstream oFile1;
		oFile1.open(fileName1, ios::app);
		oFile1 << ID1 << "\n" << read1 << "\n+\n" << qual1 << "\n";
		oFile1.close();*/
		
		stringstream ofr1pn;  //This is the correct way to do it.
		ofr1pn << "./results/curjob/ind/read1Pass_p" << tNum << ".fastq";
		ofstream ofr1ps;
		ofr1ps.open(ofr1pn.str(), ios::app);
		ofr1ps << ID1 << "\n" << read1 << "\n+\n" << qual1 << "\n";
		ofr1ps.close();

		stringstream ofr2pn;
		ofr2pn << "./results/curjob/ind/read2Pass_p" << tNum << ".fastq";
		ofstream ofr2ps;
		ofr2ps.open(ofr2pn.str(), ios::app);
		ofr2ps << ID2 << "\n" << read2 << "\n+\n" << qual2 << "\n";
		ofr2ps.close();
		
	} else {
		stringstream ofsrpn;
		ofsrpn << "./results/curjob/ind/singleReadPass_p" << tNum << ".fastq";
		ofstream ofsrps;
		ofsrps.open(ofsrpn.str(), ios::app);
		ofsrps << ID1 << "\n" << fRead << "\n+\n" << fQual << "\n";
		ofsrps.close();
		
		if (badSide == 2) {
			stringstream ofr2fn;
			ofr2fn << "./results/curjob/ind/read2Fail_p" << tNum << ".fastq";
			ofstream ofr2fs;
			ofr2fs.open(ofr2fn.str(), ios::app);
			ofr2fs << ID2 << "\n" << read2 << "\n+\n" << qual2 << "\n";
			ofr2fs.close();
			
		} else if (badSide == 1) {
			stringstream ofr1fn;
			ofr1fn << "./results/curjob/ind/read1Fail_p" << tNum << ".fastq";
			ofstream ofr1fs;
			ofr1fs.open(ofr1fn.str(), ios::app);
			ofr1fs << ID1 << "\n" << read1 << "\n+\n" << qual1 << "\n";
			ofr1fs.close();
		}
	}
}

void ReadPair::failOutFile() {
	if(fRead.compare("") == 0) {
		stringstream ofr1fn;
		ofr1fn << "./results/curjob/ind/read1Fail_p" << tNum << ".fastq";
		ofstream ofr1fs;
		ofr1fs.open(ofr1fn.str(), ios::app);
		ofr1fs << ID1 << "\n" << read1 << "\n+\n" << qual1 << "\n";
		ofr1fs.close();

		stringstream ofr2fn;
		ofr2fn << "./results/curjob/ind/read2Fail_p" << tNum << ".fastq";
		ofstream ofr2fs;
		ofr2fs.open(ofr2fn.str(), ios::app);
		ofr2fs << ID2 << "\n" << read2 << "\n+\n" << qual2 << "\n";
		ofr2fs.close();

	} else {
		stringstream ofsrfn;
		ofsrfn << "./results/curjob/ind/singleReadFail_p" << tNum << ".fastq";
		ofstream ofsrfs;
		ofsrfs.open(ofsrfn.str(), ios::app);
		ofsrfs << ID1 << "\n" << fRead << "\n+\n" << fQual << "\n";
		ofsrfs.close();
	}
}

int ReadPair::getLeftA() {
	return lAdap;
}
int ReadPair::getLeftAL() {
	return lAdapLength;
}
int ReadPair::getRightA() {
	return rAdap;
}
int ReadPair::getRightAL() {
	return rAdapLength;
}
int ReadPair::getBad() {
	return badRead;
}
int ReadPair::isMerged() {
	return merged;
}
int ReadPair::tStripped() {
	return tRem;
}

void ReadPair::Compile() {
	tStrip();
	aRemove();
	oCheck();
	int p = qualPass();
	if(p == 3) passOutFile();
	else failOutFile();
}



