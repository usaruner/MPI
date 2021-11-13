#include <mpi.h>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
const static int ARRAY_SIZE = 130000;
using Lines = char[ARRAY_SIZE][16];

/************* Feel free to change code below (even main() function), add functions, etc.. But do not change CL arguments *************/


struct letter_only: std::ctype<char> 
{
    letter_only(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask> 
            rc(std::ctype<char>::table_size,std::ctype_base::space);

        std::fill(&rc['A'], &rc['z'+1], std::ctype_base::alpha);
        return &rc[0];
    }
};

void DoOutput(std::string word, int result)
{
    std::cout << "Word Frequency: " << word << " -> " << result << std::endl;
}


int countFrequency( std::vector<std::string> data, std::string word)
{
	int freq = 0;
	for(auto workString : data) {
        //std::cout << workString << " ";
        if (!workString.compare(word)) {
            freq++;
            
        }
	}
	return freq;
}
/*
int countFrequency(std::string data, std::string word)
{
    int freq = 0;
    for (auto workString : data) {
        if (!workString.compare(word))
            freq++;
    }
    return freq;
}
*/
int main(int argc, char* argv[])
{
    int processId;
    int numberOfProcesses;
    int *to_return = NULL;
    double start_time, end_time;
    int local_sum = 0;
    int global_sum = 0;
    int numwords = 0;
    int prev, next;
    int tag1 = 1;
    int tag2 = 2;
    int prevsum;
    std::string words[10] = { "a","to", "he" , "of", "and", "her", "though", "however" ,"herself","elizabeth" };
    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &numberOfProcesses);
 
    // Two arguments, the program name and the input file. The second should be the input file
    if(argc != 4)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <path to search file> <search word> <b1/b2>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }
	std::string word = argv[2];
 
    Lines lines;
    if (processId == 0) {
        std::ifstream file;
		file.imbue(std::locale(std::locale(), new letter_only()));
		file.open(argv[1]);
		std::string workString;
		int i = 0;
		while(file >> workString){
			memset(lines[i], '\0', 16);
			memcpy(lines[i++], workString.c_str(), workString.length());
            numwords++;
		}
        memset(lines[i], '\0', 16);
    }
    /*
    for (int i = 0; i < numwords; i++) {
        std::cout << lines[i] << std::endl;
    }*/
    char buf[(ARRAY_SIZE / numberOfProcesses) * 16];
    //std::cout << "size: " << (ARRAY_SIZE / numberOfProcesses) * 16 << std::endl;
    MPI_Scatter(lines, (ARRAY_SIZE / numberOfProcesses) * 16, MPI::CHAR, buf, (ARRAY_SIZE / numberOfProcesses) * 16, MPI::CHAR, 0, MPI_COMM_WORLD);
    //std::cout << "size: " << (ARRAY_SIZE / numberOfProcesses) * 16 << std::endl;
    
    
    std::vector<std::string> data;
    for (int i = 0; i < (ARRAY_SIZE / numberOfProcesses) * 16; i += 16) {
        char s[16];
        memcpy(s, &buf[i], 16);
        std::string s2(s);
        data.push_back(s2);
    }
    int index = numberOfProcesses * (((int)numwords / numberOfProcesses));
    //std::cout << "rank " << processId;
    //std::cout << "numword: " << numwords;
    while (index < numwords) {
        //std::cout << "rank " << processId;
        char s[16];
        memcpy(s, &lines[index], 16);
        std::string s2(s);
        data.push_back(s2);
        index++;
    }
    //for loop to run words[] change word to words[]
    //for (int i = 0; i < 10; i++) {
        start_time = MPI_Wtime();
        int temp_result = countFrequency(std::ref(data), word);
        local_sum = countFrequency(std::ref(data), word);
        //std::cout << "result: " << temp_result << std::endl;
        std::string opt = argv[3];
        if (!opt.compare("b1"))
        {
            //std::cout << "Using reduction" << std::endl;
            /*for () {
                local_sum = countFrequency(std::ref(data), word);
            }*/
            MPI_Reduce(&local_sum, &global_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
            to_return = &global_sum;

        }
        else {
            //std::cout << "Using ring" << processId<< std::endl;
            prev = processId - 1;
            next = processId + 1;
            if (processId == 0)
                prev = processId - 1;
            if (processId == (numberOfProcesses - 1))
                next = 0;
            if (processId == 0) {
                //std::cout << "Send: " << processId << " to " << next << std::endl;
                MPI_Send(&local_sum, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
                MPI_Recv(&prevsum, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                //std::cout << "RCV: " << processId << " from " << prev << std::endl;
                global_sum = prevsum;
            }
            else {
                //std::cout << "RCV: " << processId << " from " << prev << std::endl;
                MPI_Recv(&prevsum, 1, MPI_INT, prev, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                local_sum += prevsum;
                //std::cout << "Send: " << processId << " to " << next << std::endl;
                MPI_Send(&local_sum, 1, MPI_INT, next, 0, MPI_COMM_WORLD);
            }
            to_return = &global_sum;
        }
        // ... Eventually.. 
        if (processId == 0)
        {
            DoOutput(word, *to_return);
            end_time = MPI_Wtime();
            std::cout << "time: " << ((double)end_time - start_time) << std::endl;
        }
    //}
    MPI_Finalize();
 
    return 0;
}
