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
