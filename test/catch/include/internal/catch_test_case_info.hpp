/*
 *  Created by Phil on 14/08/2012.
 *  Copyright 2012 Two Blue Cubes Ltd. All rights reserved.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef TWOBLUECUBES_CATCH_TESTCASEINFO_HPP_INCLUDED
#define TWOBLUECUBES_CATCH_TESTCASEINFO_HPP_INCLUDED

#include "catch_test_case_info.h"
#include "catch_interfaces_testcase.h"

namespace Catch {

    TestCaseInfo::TestCaseInfo( ITestCase* testCase,
                                const char* name,
                                const char* description,
                                const SourceLineInfo& lineInfo )
    :   m_test( testCase ),
        m_name( name ),
        m_description( description ),
        m_lineInfo( lineInfo )
    {}

    TestCaseInfo::TestCaseInfo()
    :   m_test( NULL ),
        m_name(),
        m_description()
    {}

    TestCaseInfo::TestCaseInfo( const TestCaseInfo& other, const std::string& name )
    :   m_test( other.m_test ),
        m_name( name ),
        m_description( other.m_description ),
        m_lineInfo( other.m_lineInfo )
    {}

    TestCaseInfo::TestCaseInfo( const TestCaseInfo& other )
    :   m_test( other.m_test ),
        m_name( other.m_name ),
        m_description( other.m_description ),
        m_lineInfo( other.m_lineInfo )
    {}

    void TestCaseInfo::invoke() const {
        m_test->invoke();
    }

    const std::string& TestCaseInfo::getName() const {
        return m_name;
    }

    const std::string& TestCaseInfo::getDescription() const {
        return m_description;
    }

    const SourceLineInfo& TestCaseInfo::getLineInfo() const {
        return m_lineInfo;
    }

    bool TestCaseInfo::isHidden() const {
        return m_name.size() >= 2 && m_name[0] == '.' && m_name[1] == '/';
    }

    void TestCaseInfo::swap( TestCaseInfo& other ) {
        m_test.swap( other.m_test );
        m_name.swap( other.m_name );
        m_description.swap( other.m_description );
        m_lineInfo.swap( other.m_lineInfo );
    }

    bool TestCaseInfo::operator == ( const TestCaseInfo& other ) const {
        return m_test.get() == other.m_test.get() && m_name == other.m_name;
    }

    bool TestCaseInfo::operator < ( const TestCaseInfo& other ) const {
        return m_name < other.m_name;
    }
    TestCaseInfo& TestCaseInfo::operator = ( const TestCaseInfo& other ) {
        TestCaseInfo temp( other );
        swap( temp );
        return *this;
    }
}

#endif // TWOBLUECUBES_CATCH_TESTCASEINFO_HPP_INCLUDED
