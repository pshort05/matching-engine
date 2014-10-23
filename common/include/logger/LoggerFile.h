/*
* Copyright (C) 2014, Fabien Aulaire
* All rights reserved.
*/

#ifndef LOGGERFILE_HH
#define LOGGERFILE_HH

#include <algorithm>
#include <string>
#include <fstream>

#include "ILogger.h"

#include <boost/date_time/posix_time/posix_time.hpp>

namespace exchange
{

    namespace common
    {

        namespace logger
        {

            class LoggerFile : public ILogger
            {
            public:

                LoggerFile();

                virtual ~LoggerFile();

                virtual bool Init(const parser_type & Config);

                virtual void write(const std::string& msg);

            private:

                std::ofstream                   m_File;
                std::string                     m_FilePath;
                boost::posix_time::time_facet * m_facet;
            };
        }
    }

}

#endif // LOGGERFILE_HH