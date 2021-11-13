a#include <algorithm>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdio.h>
#include "mpi.h"

 
/* Global variables, Look at their usage in main() */
int image_height;
int image_width;
int image_maxShades;
int** inputImage;
int hist[256];
int answer[256];
int col;
int row;
int matrix[100][100];
bool flag = false;
int recbuf[256];
int empty[256];

std::string str;
std::string* strarr;
/* ****************Need not to change the function below ***************** */

int main(int argc, char* argv[])
{
    int numtasks, rank, next, prev, buf[2], tag1 = 1, tag2 = 2;
    //MPI_Request reqs[4]; // required variable for non-blocking calls
    //MPI_Status stats[4]; // required variable for Waitall routine
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (int i = 0; i < 256; i++) {
        empty[i] = 0;
        answer[i] = 0;
        hist[i] = 0;
    }
    if (argc != 4)
    {
        std::cout << "ERROR: Incorrect number of arguments. Format is: <Input image filename> <adjacency matrix> <Output image filename>" << std::endl;
        return 0;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open())
    {
        std::cout << "ERROR: Could not open file " << argv[1] << std::endl;
        return 0;
    }

    //setup matrix
    std::ifstream mfile(argv[2]);
    row = 0;
    col = 0;
    std::string token;
    size_t pos = 0;
    if (mfile.is_open())
    {
        const char* temp;
        int i = 0;
        int j = 0;
        while (getline(mfile, str)) {
            //std::cout << "str[" << str << std::endl;
            while ((pos = str.find(" ")) != std::string::npos) {
                //std::cout << "str[" << i << "]" << "[" << j << "]" << str << std::endl;
                token = str.substr(0, pos);
                temp = token.c_str();
                matrix[i][j] = atoi(temp);
                str.erase(0, pos + 1);
                i++;
                if (col < i) {
                    col = i;
                }
            }
            temp = str.c_str();
            matrix[i][j] = atoi(temp);
            i = 0;
            j++;
            if (row < j) {
                row = j;
            }
        }
        col++;
        /*
        std::cout << "opened matrix " << col << "x" << row << argv[2] << std::endl;
        for (int i = 0; i < col; i++) {
            for (int j = 0; j < row; j++) {
                std::cout << matrix[i][j] << " ";
            }
            std::cout << std::endl;
        }
        for (int i = 0; i < col; i++) {
            for (int j = 0; j < row; j++) {
                std::cout << initmatrix[i][j] << " ";
            }
            std::cout << std::endl;
        }
        */
    }
    else {
        std::cout << "ERROR: Could not open file " << argv[2] << std::endl;
        return 0;
    }
    /*****Reading image into 2-D array below******** */

    std::string workString;
    /* Remove comments '#' and check image format */
    while (std::getline(file, workString))
    {
        if (workString.at(0) != '#') {
            if (workString.at(1) != '2') {
                std::cout << "Input image is not a valid PGM image" << std::endl;
                return 0;
            }
            else {
                break;
            }
        }
        else {
            continue;
        }
    }
    /* Check image size */
    while (std::getline(file, workString))
    {
        if (workString.at(0) != '#') {
            std::stringstream stream(workString);
            int n;
            stream >> n;
            image_width = n;
            stream >> n;
            image_height = n;
            break;
        }
        else {
            continue;
        }
    }

    inputImage = new int* [image_height];
    for (int i = 0; i < image_height; ++i) {
        inputImage[i] = new int[image_width];
    }

    /* Check image max shades */
    while (std::getline(file, workString))
    {
        if (workString.at(0) != '#') {
            std::stringstream stream(workString);
            stream >> image_maxShades;
            break;
        }
        else {
            continue;
        }
    }
    /* Fill input image matrix */
    int pixel_val;
    for (int i = 0; i < image_height; i++)
    {
        if (std::getline(file, workString) && workString.at(0) != '#') {
            std::stringstream stream(workString);
            for (int j = 0; j < image_width; j++) {
                if (!stream)
                    break;
                stream >> pixel_val;
                inputImage[i][j] = pixel_val;
            }
        }
        else {
            continue;
        }
    }



    /*for (int i = 0; i < image_height; i++) {
        for (int j = 0; j < image_width; j++) {
            std::cout << inputImage[i][j];
        }
        std::cout << std::endl;
    }*/



    int last = 0;
    int tempint;


    //std::cout << "Rank " << rank << " length: " << image_width << std::endl;

   // std::cout << std::endl;
    /*
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            std::cout << matrix[i][j] << " ";
        }
        std::cout << std::endl;
    }
    for (int i = 0; i < col; i++) {
        for (int j = 0; j < row; j++) {
            std::cout << initmatrix[i][j] << " ";
        }
        std::cout << std::endl;
    }*/
    int traveled[row];
    for (int i = 0; i < row; i++) {
        traveled[row] = 0;
    }
    int tempbuf[image_width / numtasks];
    for (int i = 0; i < image_width / numtasks; i++) {
        tempbuf[i] = 0;
    }
    /*
    if (rank == 2) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < image_width; j++) {
                std::cout << ", " << inputImage[i][j];
            }
        }
    }
    */
    //std::cout <<"--------------------------------------------------\n";
    /*
    for (int i = 0; i < image_width; i++) {
        std::cout << ", " << inputImage[0][i];
    }
    */
    //std::cout << "\n\n\n";
    for (int i = 0; i < image_height; i++) {
        MPI_Scatter(inputImage[i], image_width / numtasks, MPI_INT, tempbuf, image_width / numtasks, MPI_INT, 2, MPI_COMM_WORLD);
        for (int j = 0; j < image_width / 4; j++) {
            //std::cout << ", " << tempbuf[j];
            hist[tempbuf[j]]++;
        }
        if (rank == 2) {
            last = 4 * (int)((image_width / numtasks));
            while (last != image_width) {
                hist[inputImage[i][last]]++;
                last++;
            }
        }
    }
    /*
    std::cout << "hist: "<< rank << std::endl;
    for (int i = 0; i < 256; i++) {
        if (hist[i] != 0) {
            std::cout << "hist[ " << i << " ]:" << hist[i] << "\n" ;
        }
    }
    */
    
    //Addition
    if (rank == 2) {
        for (int j = 0; j < row; j++) {
            if (matrix[rank][j] == 1 && traveled[j] == 0) {
                std::cout << "Send: " << rank << " to " << j << std::endl;
                MPI_Send(hist, 256, MPI_INT, j,0, MPI_COMM_WORLD);
                MPI_Send(traveled, row, MPI_INT, j, 0, MPI_COMM_WORLD);
                MPI_Recv(recbuf, 256, MPI_INT, j,0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(traveled, row, MPI_INT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                std::cout << "RCV: " << rank << " from " << j << std::endl;
                for (int i = 0; i < 256; i++) {
                    hist[i] += recbuf[i];
                    recbuf[i] = 0;
                }
            }
        }
    }
    else {
        MPI_Status status;
        MPI_Recv(recbuf, 256, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(traveled, row, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        traveled[rank] = 1;

        std::cout << "unblock" <<std::endl;
        int initsender = status.MPI_SOURCE;
        std::cout << "RCV: " << rank << " from " << initsender << std::endl;
        std::cout << "flag: " << flag << " initsender: "<< initsender <<std::endl;

        if (flag == false ) {
            for (int j = 0; j < row; j++) {
                if (matrix[rank][j] == 1 && traveled[j] == 0) {
                    std::cout << "Send: " << rank << " to " << j << std::endl;
                    MPI_Send(hist, 256, MPI_INT, j,0, MPI_COMM_WORLD);
                    MPI_Send(traveled, row, MPI_INT, j, 0, MPI_COMM_WORLD);
                    MPI_Recv(recbuf, 256, MPI_INT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(traveled, row, MPI_INT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    std::cout << "RCV: " << rank << " from " << j << std::endl;
                    for (int i = 0; i < 256; i++) {
                        hist[i] += recbuf[i];
                        recbuf[i] = 0;
                    }
                }
            }
            flag = true;
            std::cout << "Send: " << rank << " to " << initsender << std::endl;
            MPI_Send(hist, 256, MPI_INT, initsender, 0, MPI_COMM_WORLD);
            MPI_Send(traveled, row, MPI_INT, initsender, 0, MPI_COMM_WORLD);
        }
        else {
            std::cout << "Send empty: " << rank << " to " << MPI_ANY_SOURCE << std::endl;
            MPI_Send(empty, 256, MPI_INT, initsender, 0, MPI_COMM_WORLD);
            MPI_Send(traveled, row, MPI_INT, initsender, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    if (rank == 2) {
        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                answer[inputImage[j][i]]++;
            }
        }
        std::ofstream ofile(argv[3]);
        if (ofile.is_open()) {
            for (int i = 0; i < 256; i++) {
                ofile << i << " : " << hist[i] << " answer " << " : " << answer[i] << "\n";
            }
        }
        else {
            std::cout << "ERROR: Could not open output file " << argv[3] << std::endl;
            return 0;
        }
        /*
        for (int j = 0; j < 256; j++) {
            std::cout << "answer[" << j << "] = " << answer[j] << " vs " << hist[j] << std::endl;
        }*/
    }
    //MPI_Finalize();
}
