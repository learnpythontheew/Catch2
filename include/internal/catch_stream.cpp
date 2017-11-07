/*
 *  Created by Phil on 17/01/2011.
 *  Copyright 2011 Two Blue Cubes Ltd. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#include "catch_common.h"
#include "catch_enforce.h"
#include "catch_stream.h"
#include "catch_debug_console.h"
#include "catch_stringref.h"

#include <stdexcept>
#include <cstdio>
#include <iostream>
#include <fstream>

namespace Catch {

    Catch::IStream::~IStream() = default;

    namespace detail { namespace {
        template<typename WriterF, std::size_t bufferSize=256>
        class StreamBufImpl : public std::streambuf {
            char data[bufferSize];
            WriterF m_writer;

        public:
            StreamBufImpl() {
                setp( data, data + sizeof(data) );
            }

            ~StreamBufImpl() noexcept {
                StreamBufImpl::sync();
            }

        private:
            int overflow( int c ) override {
                sync();

                if( c != EOF ) {
                    if( pbase() == epptr() )
                        m_writer( std::string( 1, static_cast<char>( c ) ) );
                    else
                        sputc( static_cast<char>( c ) );
                }
                return 0;
            }

            int sync() override {
                if( pbase() != pptr() ) {
                    m_writer( std::string( pbase(), static_cast<std::string::size_type>( pptr() - pbase() ) ) );
                    setp( pbase(), epptr() );
                }
                return 0;
            }
        };

        ///////////////////////////////////////////////////////////////////////////

        struct OutputDebugWriter {

            void operator()( std::string const&str ) {
                writeToDebugConsole( str );
            }
        };

        ///////////////////////////////////////////////////////////////////////////

        class FileStream : public IStream {
            mutable std::ofstream m_ofs;
        public:
            FileStream( StringRef filename ) {
                m_ofs.open( filename.c_str() );
                CATCH_ENFORCE( !m_ofs.fail(), "Unable to open file: '" << filename << "'" );
            }
            ~FileStream() override = default;
        public: // IStream
            std::ostream& stream() const override {
                return m_ofs;
            }
        };

        ///////////////////////////////////////////////////////////////////////////

        class CoutStream : public IStream {
            mutable std::ostream m_os;
        public:
            // Store the streambuf from cout up-front because
            // cout may get redirected when running tests
            CoutStream() : m_os( Catch::cout().rdbuf() ) {}
            ~CoutStream() override = default;

        public: // IStream
            std::ostream& stream() const override { return m_os; }
        };

        ///////////////////////////////////////////////////////////////////////////

        class DebugOutStream : public IStream {
            std::unique_ptr<StreamBufImpl<OutputDebugWriter>> m_streamBuf;
            mutable std::ostream m_os;
        public:
            DebugOutStream()
            :   m_streamBuf( new StreamBufImpl<OutputDebugWriter>() ),
                m_os( m_streamBuf.get() )
            {}

            ~DebugOutStream() override = default;

        public: // IStream
            std::ostream& stream() const override { return m_os; }
        };

    }} // namespace anon::detail

    ///////////////////////////////////////////////////////////////////////////

    auto makeStream( StringRef const &filename ) -> IStream const* {
        if( filename.empty() )
            return new detail::CoutStream();
        else if( filename[0] == '%' ) {
            if( filename == "%debug" )
                return new detail::DebugOutStream();
            else
                CATCH_ERROR( "Unrecognised stream: '" << filename << "'" );
        }
        else
            return new detail::FileStream( filename );
    }

    ///////////////////////////////////////////////////////////////////////////


#ifndef CATCH_CONFIG_NOSTDOUT // If you #define this you must implement these functions
    std::ostream& cout() {
        return std::cout;
    }
    std::ostream& cerr() {
        return std::cerr;
    }
    std::ostream& clog() {
        return std::clog;
    }
#endif
}