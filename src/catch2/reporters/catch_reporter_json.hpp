
//              Copyright Catch2 Authors
// Distributed under the Boost Software License, Version 1.0.
//   (See accompanying file LICENSE.txt or copy at
//        https://www.boost.org/LICENSE_1_0.txt)

// SPDX-License-Identifier: BSL-1.0

#ifndef CATCH_REPORTER_JSON_HPP_INCLUDED
#define CATCH_REPORTER_JSON_HPP_INCLUDED

#include <catch2/catch_timer.hpp>
#include <catch2/internal/catch_jsonwriter.hpp>
#include <catch2/reporters/catch_reporter_streaming_base.hpp>

#include <stack>

namespace Catch {
    class JsonReporter : public StreamingReporterBase {
    public:
        JsonReporter( ReporterConfig&& config );

        ~JsonReporter() override;

        static std::string getDescription();

        void writeSourceInfo( SourceLineInfo const& sourceInfo );

    public: // StreamingReporterBase
        void testRunStarting( TestRunInfo const& testInfo ) override;

        void testCaseStarting( TestCaseInfo const& testInfo ) override;

        void sectionStarting( SectionInfo const& sectionInfo ) override;

        void assertionStarting( AssertionInfo const& ) override;

        void assertionEnded( AssertionStats const& assertionStats ) override;

        void sectionEnded( SectionStats const& sectionStats ) override;

        void testCaseEnded( TestCaseStats const& testCaseStats ) override;

        void testRunEnded( TestRunStats const& testRunStats ) override;

        void benchmarkPreparing( StringRef name ) override;
        void benchmarkStarting( BenchmarkInfo const& ) override;
        void benchmarkEnded( BenchmarkStats<> const& ) override;
        void benchmarkFailed( StringRef error ) override;

        void listReporters(
            std::vector<ReporterDescription> const& descriptions ) override;
        void listListeners(
            std::vector<ListenerDescription> const& descriptions ) override;
        void listTests( std::vector<TestCaseHandle> const& tests ) override;
        void listTags( std::vector<TagInfo> const& tags ) override;

    private:
        Timer m_testCaseTimer;
        enum class Writer {
            Object,
            Array
        };

        void pushArray();
        void pushArray( std::string const& key );

        void pushObject();
        void pushObject( std::string const& key );

        bool isInside( Writer writer );

        // Invariant:
        // When m_writers is not empty and its top element is
        // - Writer::Object, then m_objectWriters is not be empty
        // - Writer::Array,  then m_arrayWriters shall not be empty
        std::stack<JsonObjectWriter> m_objectWriters{};
        std::stack<JsonArrayWriter> m_arrayWriters{};
        std::stack<Writer> m_writers{};
    };
} // namespace Catch

#endif // CATCH_REPORTER_JSON_HPP_INCLUDED
