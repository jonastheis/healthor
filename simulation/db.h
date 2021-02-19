#ifndef DB_H_
#define DB_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include <iostream>
#include <iomanip>

#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "rocksdb/iterator.h"
#include "rocksdb/utilities/options_util.h"

#include "cereal/archives/binary.hpp"
#include "local_datatypes.h"

using namespace omnetpp;

// 5=up to 10k nodes, 1=prefix for kind of store
static const int PREFIX_SIZE = 5 + 1;
static const int KEY_SIZE = PREFIX_SIZE + ID_SIZE;

class DBWrapper {
public:
    DBWrapper() {}

    DBWrapper(const char* path) {
        // clean up from previous runs
        char deleteCmd[500];
        sprintf(deleteCmd, "rm -r %s", path);
        std::system(deleteCmd);

        rocksdb::Options options;
        // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
        options.IncreaseParallelism();
        options.OptimizeLevelStyleCompaction();
        // create bigger cache
//        auto cache = rocksdb::NewLRUCache(4 * 1024 * 1024 * 1024); // 1 GB
//        rocksdb::BlockBasedTableOptions bbt_opts;
//        bbt_opts.block_cache = cache;
//        options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(bbt_opts));

        // create the DB if it's not already present
        options.create_if_missing = true;
        options.error_if_exists = true;

        // define for prefix seek
        options.prefix_extractor.reset(rocksdb::NewCappedPrefixTransform(PREFIX_SIZE));

        s = rocksdb::DB::Open(options, path, &db);
        if(!s.ok()) {
            EV_ERROR << "Could not create DB at " << path << "\n";
        }
    }

    void iterateTangle(int nodeId, std::function<void(LocalTangleMessage*)> callback) {
        // we want to get all messages of the local tangle of a node
        std::string prefixString = makeFixedLength(nodeId, 5) + 't';
        rocksdb::Slice prefix = rocksdb::Slice(prefixString);

        std::string upperString = prefixString + 'z';
        rocksdb::Slice upper = rocksdb::Slice(upperString);

//        EV << prefix.ToString() << " " << upper.ToString() << "\n";
        // special options to use seek with prefix (fixed length)
        rocksdb::ReadOptions read_options;
        read_options.auto_prefix_mode = true;
        read_options.iterate_upper_bound = &upper;

        rocksdb::Iterator* iter = db->NewIterator(read_options);
        iter->Seek(prefix);

        LocalTangleMessage msg;
        while(iter->Valid()) {
            rocksdb::Slice key = iter->key();
            rocksdb::Slice value = iter->value();

            // deserialize
            std::stringstream stream(value.ToString());
            cereal::BinaryInputArchive iarchive(stream);
            iarchive(msg);

            callback(&msg);

            iter->Next();
        }
        delete iter;
    }

    void putKnownMessage(int nodeId, LocalTangleMessage *msg) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msg->id, 'k');

        putMessage(prefixedId, msg);
    }

    LocalTangleMessage* getKnownMessage(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 'k');

        return getMessage(prefixedId, msgId);
    }

    void deleteKnownMessage(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 'k');

        s = db->Delete(rocksdb::WriteOptions(), prefixedId);
        if(!s.ok()) {
            EV_ERROR << "Could not delete LocalTangleMessage " << msgId << " with prefix: " << prefixedId << "\n";
        }
    }

    void putTangleMessage(int nodeId, LocalTangleMessage *msg) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msg->id, 't');

        putMessage(prefixedId, msg);
    }

    LocalTangleMessage* getTangleMessage(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 't');

        return getMessage(prefixedId, msgId);
    }

    bool isInTangle(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 't');

        std::string value;
        s = db->Get(rocksdb::ReadOptions(), prefixedId, &value);

        return s.ok();
    }

    void putSeen(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 's');

        s = db->Put(rocksdb::WriteOptions(), prefixedId, NULL);
        if(!s.ok()) {
            EV_ERROR << "Could not store Seen " << msgId << " with prefix: " << prefixedId << "\n";
        }
    }

    bool isSeen(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 's');

        std::string value;
        s = db->Get(rocksdb::ReadOptions(), prefixedId, &value);

        return s.ok();
    }

    void deleteSeen(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 's');

        s = db->Delete(rocksdb::WriteOptions(), prefixedId);
        if(!s.ok()) {
            EV_ERROR << "Could not delete Seen " << msgId << " with prefix: " << prefixedId << "\n";
        }
    }

    void putApprovers(int nodeId, Approvers* a) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, a->id, 'a');

        std::stringstream stream; // any stream can be used
        {
            // write data to stream
            cereal::BinaryOutputArchive oarchive(stream);

            oarchive(*a);
        } // archive goes out of scope, ensuring all contents are flushed

        // write data to KV store
        s = db->Put(rocksdb::WriteOptions(), prefixedId, stream.str());
        if(!s.ok()) {
            EV_ERROR << "Could not store Approvers " << a->id << " with prefix: " << prefixedId << "\n";
        }
    }

    Approvers* getApprovers(int nodeId, const char *msgId) {
        char prefixedId[KEY_SIZE];
        prefix(prefixedId, nodeId, msgId, 'a');

        std::string value;
        s = db->Get(rocksdb::ReadOptions(), prefixedId, &value);
        if(s.IsNotFound()) {
            return NULL;
        }
        if(!s.ok()) {
            EV_ERROR << "Could not load Approvers " << msgId << " with prefix: " << prefixedId << "\n";
            return NULL;
        }

        std::stringstream stream(value);

        cereal::BinaryInputArchive iarchive(stream);

        Approvers a;
        iarchive(a);

        Approvers* a2 = new Approvers();

        memcpy(a2->id, a.id, ID_SIZE);
        a2->a = std::set<std::string>(a.a);

        return a2;
    }

private:
    rocksdb::DB* db;
    rocksdb::Status s;

    std::string makeFixedLength(const int i, const int length) {
        std::ostringstream ostr;
        ostr << std::setfill('0') << std::setw(length) << i;
        return ostr.str();
    }


    void prefix(char* prefixed, int nodeId, const char* key, const char prefix) {
        sprintf(prefixed, "%s%c%s", makeFixedLength(nodeId, 5).c_str(), prefix, key);
    }

    void putMessage(const char* key, LocalTangleMessage *msg) {
        std::stringstream stream; // any stream can be used
        {
            // write data to stream
            cereal::BinaryOutputArchive oarchive(stream);

            oarchive(*msg);
        } // archive goes out of scope, ensuring all contents are flushed

        // write data to KV store
        s = db->Put(rocksdb::WriteOptions(), key, stream.str());
        if(!s.ok()) {
            EV_ERROR << "Could not store LocalTangleMessage " << msg->id << " with prefix: " << key << "\n";
        }
    }

    LocalTangleMessage* getMessage(const char *key, const char* msgId) {
        std::string value;
        s = db->Get(rocksdb::ReadOptions(), key, &value);
        if(!s.ok()) {
            EV_ERROR << "Could not load LocalTangleMessage " << msgId << " with prefix: " << key << "\n";
            return NULL;
        }

        std::stringstream stream(value);

        cereal::BinaryInputArchive iarchive(stream);

        LocalTangleMessage msg;
        iarchive(msg);

        LocalTangleMessage* msg2 = new LocalTangleMessage();
        memcpy(msg2, &msg, sizeof(LocalTangleMessage));

        return msg2;
    }
};

#endif /* DB_H_ */
