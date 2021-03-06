#pragma once

#include <leveldb/db.h>

#include <Logger.h>

#include <string>

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>

namespace exchange
{
    namespace common
    {

        template <typename ObjectType, typename UnderlyingStorage>
        class NoSqlStorage
        {
        public:

            using key_type = typename UnderlyingStorage::key_type;

        public:

            NoSqlStorage(const std::string & DBFilePath)
                : m_UdrStorage(DBFilePath), m_DBFilePath(DBFilePath)
            {}

            ~NoSqlStorage()
            {
                m_UdrStorage.Close();
            }

            template <typename TCallback>
            bool Load(const TCallback & callback);

            bool Get(const std::string & key, ObjectType & object);

            template <typename TKeyExtractor>
            bool Write(const ObjectType & object, const TKeyExtractor & extractor, bool bSync = true, bool overwrite = false);

        protected:

            bool InitializeDB();

        private:
            
            UnderlyingStorage  m_UdrStorage;
            std::string        m_DBFilePath;
        };

        template <typename ObjectType, typename UnderlyingStorage>
        bool NoSqlStorage<ObjectType, UnderlyingStorage>::InitializeDB()
        {
            return m_UdrStorage.InitializeDB();
        }

        template <typename ObjectType, typename UnderlyingStorage>
        template <typename TCallback>
        bool NoSqlStorage<ObjectType, UnderlyingStorage>::Load(const TCallback & callback)
        {
            if (InitializeDB())
            {
                try
                {
                    auto object_handler = [&callback](const auto & value)
                    {
                        std::stringstream ss(value.ToString());
                        boost::archive::text_iarchive ia(ss);

                        ObjectType object;
                        ia >> object;

                        callback(object);
                    };

                    m_UdrStorage.Iterate(object_handler);
                    return true;
                }
                catch (const boost::archive::archive_exception & e)
                {
                    EXERR("NoSqlStorage::Load boost archive_exception : " << e.what());
                }
                catch (...)
                {
                    EXERR("NoSqlStorage::Load Unknown exception raised");
                }
            }
            return false;
        }

        template <typename ObjectType, typename UnderlyingStorage>
        bool NoSqlStorage<ObjectType, UnderlyingStorage>::Get(const std::string & key, ObjectType & object)
        {
            if (InitializeDB())
            {
                std::string result;

                const key_type  sqlkey = key;

                if (m_UdrStorage.DoRead(sqlkey, result))
                {
                    std::stringstream ss(result);
                    boost::archive::text_iarchive ia(ss);

                    ia >> object;
                    return true;
                }
                return false;
            }
            return false;
        }

        template <typename ObjectType, typename UnderlyingStorage>
        template <typename TKeyExtractor>
        bool NoSqlStorage<ObjectType, UnderlyingStorage>::Write(const ObjectType & object, const TKeyExtractor & extractor, bool bSync, bool overwrite)
        {
            if (InitializeDB())
            {
                std::stringstream stringstream;
                boost::archive::text_oarchive oa(stringstream);

                oa << object;

                const key_type  key     = extractor(object);
                const auto      svalue  = stringstream.str();

                const key_type  value   = svalue;

                if (overwrite == false)
                {
                    if (m_UdrStorage.IsExistingKey(key))
                    {
                        EXERR("NoSqlStorage::Write : Already Existing Key");
                        return false;
                    }
                }

                std::string error_msg;
                if (!m_UdrStorage.DoWrite(bSync, key, value, error_msg))
                {
                    EXERR("NoSqlStorage::Write : Failed to write to DB. Reason[" << error_msg << "]");
                    return false;
                }
                else
                {
                    EXINFO("NoSqlStorage::Write : Product[" << extractor(object) << "] written in database");
                    return true;
                }
            }
            return false;
        }

        class LevelDBStorage
        {
            public:
                
                using key_type = leveldb::Slice;

            public:

                LevelDBStorage(const std::string & DBPath)
                    :m_DBFilePath(DBPath)
                {}

                bool InitializeDB();

                template <typename TCallBack>
                void Iterate(const TCallBack & callback);

                bool IsExistingKey(const key_type & key);
                bool DoWrite(bool bSync, const key_type & key, const key_type & value, std::string & status_msg);
                bool DoRead(const key_type & key, std::string & value);

                void Close();

            private:

                const std::string   m_DBFilePath;
                leveldb::DB*        m_db = nullptr;
        };

        template <typename TCallBack>
        void LevelDBStorage::Iterate(const TCallBack & callback)
        {
            leveldb::ReadOptions options;
            options.snapshot = m_db->GetSnapshot();

            auto release_at_exit = common::make_scope_exit([this, &options]() { m_db->ReleaseSnapshot(options.snapshot); });

            leveldb::Iterator* it = m_db->NewIterator(leveldb::ReadOptions());

            for (it->SeekToFirst(); it->Valid(); it->Next())
            {
                callback(it->value());
            }

            delete it;
        }

    }
}
