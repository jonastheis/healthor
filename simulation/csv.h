#include <stdio.h>
#include <string.h>
#include <fstream>
#include <iostream>

//using namespace omnetpp;

class CSVWriter {
    public:
        CSVWriter(std::string fileDest, std::string header) {
            file.open(fileDest);
            addRow(header);
        }
        void addRow(std::string row) {
            file << row << "\n";
        }
        void close() {
            file.close();
        }

        void writeMap(std::map<std::string, int64_t> map) {

            std::map<std::string, int64_t>::iterator it;
            char buffer[100];
            for (it = map.begin(); it != map.end(); it++) {
                sprintf(buffer, "%s,%lld", it->first.c_str(), it->second);
                addRow(buffer);
            }
        }
    private:
        std::ofstream file;
};

class CSVReader {
    public:
        CSVReader(std::string fileDest) {
            file.open(fileDest);
        }

        void close() {
            file.close();
        }

        std::vector<std::string> splitRow(std::string line) {
            std::string word;
            std::vector<std::string> row;

            // used for breaking words
            std::stringstream s(line);

            // read every column, store in 'word'
            while (std::getline(s, word, ',')) {
                row.push_back(word);
            }

            return row;
        }

        std::vector<double> readRate() {
            std::vector<double> processingRate;
            std::vector<std::string> row;
            std::string line;

            int count = -1;
            while (!file.eof()) {
                // read an entire row and store in 'line'
                std::getline(file, line);
                count++;

                // skip for first line
                if(count == 0) {
                    continue;
                }

                row = splitRow(line);
                double rate = stod(row[1]);
                processingRate.push_back(rate);
            }

            return processingRate;
        }

    private:
        std::ifstream file;
};
