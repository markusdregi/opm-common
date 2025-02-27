/*
   +   Copyright 2019 Equinor ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */

#include "config.h"

#include <opm/io/eclipse/ERst.hpp>

#define BOOST_TEST_MODULE Test EclIO
#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/EclOutput.hpp>
#include <opm/io/eclipse/OutputStream.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <math.h>
#include <stdio.h>
#include <tuple>
#include <type_traits>

#include <boost/filesystem.hpp>

using namespace Opm::EclIO;

template<typename InputIterator1, typename InputIterator2>
bool
range_equal(InputIterator1 first1, InputIterator1 last1,
            InputIterator2 first2, InputIterator2 last2)
{
    while(first1 != last1 && first2 != last2)
    {
        if(*first1 != *first2) return false;
        ++first1;
        ++first2;
    }
    return (first1 == last1) && (first2 == last2);
}

bool compare_files(const std::string& filename1, const std::string& filename2)
{
    std::ifstream file1(filename1);
    std::ifstream file2(filename2);

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    std::istreambuf_iterator<char> end;

    return range_equal(begin1, end, begin2, end);
}


template <typename T>
bool operator==(const std::vector<T> & t1, const std::vector<T> & t2)
{
    return std::equal(t1.begin(), t1.end(), t2.begin(), t2.end());
}


BOOST_AUTO_TEST_CASE(TestERst_1) {

    std::string testFile="SPE1_TESTCASE.UNRST";
    std::vector<int> refReportStepNumbers= {1,2,5,10,15,25,50,100,120};

    ERst rst1(testFile);
    rst1.loadReportStepNumber(5);
    
    std::vector<int> reportStepNumbers = rst1.listOfReportStepNumbers();

    BOOST_CHECK_EQUAL(reportStepNumbers==refReportStepNumbers, true);

    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(4), false);
    BOOST_CHECK_EQUAL(rst1.hasReportStepNumber(5), true);

    // try loading non-existing report step, should throw exception
    BOOST_CHECK_THROW(rst1.loadReportStepNumber(4) , std::invalid_argument );


    // try to get a list of vectors from non-existing report step, should throw exception
    std::vector<std::tuple<std::string, eclArrType, int>> rstArrays; // = rst1.listOfRstArrays(4);
    BOOST_CHECK_THROW(rstArrays = rst1.listOfRstArrays(4), std::invalid_argument);

    // non exising report step number, should throw exception 
    
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",0) , std::invalid_argument );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",0) , std::invalid_argument );

    // report step number exists, but data is not loaded. Should throw exception
    BOOST_CHECK_THROW(std::vector<int> vect1=rst1.getRst<int>("ICON",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect2=rst1.getRst<float>("PRESSURE",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect3=rst1.getRst<double>("XGRP",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect4=rst1.getRst<bool>("LOGIHEAD",10) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<std::string> vect4=rst1.getRst<std::string>("ZWEL",10) , std::runtime_error );

    // calling getRst<T> member function with wrong type, should throw exception

    BOOST_CHECK_THROW(std::vector<float> vect1=rst1.getRst<float>("ICON",5) , std::runtime_error );
    BOOST_CHECK_THROW(std::vector<int> vect2=rst1.getRst<int>("PRESSURE",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<float> vect3=rst1.getRst<float>("XGRP",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<double> vect4=rst1.getRst<double>("LOGIHEAD",5), std::runtime_error );
    BOOST_CHECK_THROW(std::vector<bool> vect5=rst1.getRst<bool>("ZWEL",5), std::runtime_error );

    rst1.loadReportStepNumber(25);

    std::vector<int> vect1 = rst1.getRst<int>("ICON",25);
    std::vector<float> vect2 = rst1.getRst<float>("PRESSURE",25);
    std::vector<double> vect3 = rst1.getRst<double>("XGRP",25);
    std::vector<bool> vect4 = rst1.getRst<bool>("LOGIHEAD",25);
    std::vector<std::string> vect5 = rst1.getRst<std::string>("ZWEL",25);
}


static void readAndWrite(EclOutput& eclTest, ERst& rst1,
                         const std::string& name, int seqnum,
                         eclArrType arrType)
{
    if (arrType == INTE) {
        std::vector<int> vect = rst1.getRst<int>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == REAL) {
        std::vector<float> vect = rst1.getRst<float>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == DOUB) {
        std::vector<double> vect = rst1.getRst<double>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == LOGI) {
        std::vector<bool> vect = rst1.getRst<bool>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == CHAR) {
        std::vector<std::string> vect = rst1.getRst<std::string>(name, seqnum);
        eclTest.write(name, vect);
    } else if (arrType == MESS) {
        eclTest.write(name, std::vector<char>());
    } else {
        std::cout << "unknown type " << std::endl;
        exit(1);
    }
}


BOOST_AUTO_TEST_CASE(TestERst_2) {
    
    std::string testFile="SPE1_TESTCASE.UNRST";
    std::string outFile="TEST.UNRST";

    // using API for ERst to read all array from a binary unified restart file1
    // Then write the data back to a new file and check that new file is identical with input file 
    
    ERst rst1(testFile);

    {
        EclOutput eclTest(outFile, false);

        std::vector<int> seqnums = rst1.listOfReportStepNumbers();

        for (size_t i = 0; i < seqnums.size(); i++) {
            rst1.loadReportStepNumber(seqnums[i]);

            auto rstArrays = rst1.listOfRstArrays(seqnums[i]);

            for (auto& array : rstArrays) {
                std::string name = std::get<0>(array);
                eclArrType arrType = std::get<1>(array);
                readAndWrite(eclTest, rst1, name, seqnums[i], arrType);
            }
        }
    }

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str()) == -1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

BOOST_AUTO_TEST_CASE(TestERst_3) {

    std::string testFile="SPE1_TESTCASE.FUNRST";
    std::string outFile="TEST.FUNRST";

    // using API for ERst to read all array from a formatted unified restart file1
    // Then write the data back to a new file and check that new file is identical with input file 

    ERst rst1(testFile);

    EclOutput eclTest(outFile, true);

    std::vector<int> seqnums = rst1.listOfReportStepNumbers();
    for (unsigned int i=0; i<seqnums.size(); i++) {
        rst1.loadReportStepNumber(seqnums[i]);

        auto rstArrays = rst1.listOfRstArrays(seqnums[i]);

        for (auto& array : rstArrays) {
            std::string name = std::get<0>(array);
            eclArrType arrType = std::get<1>(array);
            readAndWrite(eclTest, rst1, name, seqnums[i], arrType);
        }
    }

    BOOST_CHECK_EQUAL(compare_files(testFile, outFile), true);

    if (remove(outFile.c_str() )== -1) {
        std::cout << " > Warning! temporary file was not deleted" << std::endl;
    };
}

// ====================================================================

class RSet
{
public:
    explicit RSet(std::string base)
        : odir_(boost::filesystem::temp_directory_path() /
                boost::filesystem::unique_path("rset-%%%%"))
        , base_(std::move(base))
    {
        boost::filesystem::create_directories(this->odir_);
    }

    ~RSet()
    {
        boost::filesystem::remove_all(this->odir_);
    }

    operator ::Opm::EclIO::OutputStream::ResultSet() const
    {
        return { this->odir_.string(), this->base_ };
    }

private:
    boost::filesystem::path odir_;
    std::string             base_;
};

namespace {
    template <class Coll>
    void check_is_close(const Coll& c1, const Coll& c2)
    {
        using ElmType = typename std::remove_cv<
            typename std::remove_reference<
                typename std::iterator_traits<
                    decltype(std::begin(c1))
                >::value_type
            >::type
        >::type;

        for (auto b1  = c1.begin(), e1 = c1.end(), b2 = c2.begin();
                  b1 != e1; ++b1, ++b2)
        {
            BOOST_CHECK_CLOSE(*b1, *b2, static_cast<ElmType>(1.0e-7));
        }
    }
} // Anonymous namespace

namespace Opm { namespace EclIO {

    // Needed by BOOST_CHECK_EQUAL_COLLECTIONS.
    std::ostream&
    operator<<(std::ostream& os, const EclFile::EclEntry& e)
    {
        os << "{ " << std::get<0>(e)
           << ", " << static_cast<int>(std::get<1>(e))
           << ", " << std::get<2>(e)
           << " }";

        return os;
    }
}} // Namespace Opm::EclIO

BOOST_AUTO_TEST_SUITE(Separate)

BOOST_AUTO_TEST_CASE(Unformatted)
{
    const auto rset = RSet("CASE");
    const auto fmt  = ::Opm::EclIO::OutputStream::Formatted{ false };
    const auto unif = ::Opm::EclIO::OutputStream::Unified  { false };

    {
        const auto seqnum = 1;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 7, 2, 9});
        rst.write("L", std::vector<bool>       {true, false, false, true});
        rst.write("S", std::vector<float>      {3.1f, 4.1f, 59.265f});
        rst.write("D", std::vector<double>     {2.71, 8.21});
        rst.write("Z", std::vector<std::string>{"W1", "W2"});
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "X0001");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(rst.hasReportStepNumber(1));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{1};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(1);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 4},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 3},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 2},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(1);

        {
            const auto& I = rst.getRst<int>("I", 1);
            const auto  expect_I = std::vector<int>{ 1, 7, 2, 9 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 1);
            const auto  expect_L = std::vector<bool> {
                true, false, false, true,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 1);
            const auto  expect_S = std::vector<float>{
                3.1f, 4.1f, 59.265f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 1);
            const auto  expect_D = std::vector<double>{
                2.71, 8.21,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 1);
            const auto  expect_Z = std::vector<std::string>{
                "W1", "W2",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        const auto seqnum = 5;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 2, 3, 4});
        rst.write("L", std::vector<bool>       {false, false, false, true});
        rst.write("S", std::vector<float>      {1.23e-04f, 1.234e5f, -5.4321e-9f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180});
        rst.write("Z", std::vector<std::string>{"HELLO", ", ", "WORLD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "X0005");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(!rst.hasReportStepNumber( 1));
        BOOST_CHECK( rst.hasReportStepNumber( 5));
        BOOST_CHECK(!rst.hasReportStepNumber(13));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{5};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(5);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 4},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 3},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 2},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 3},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(5);

        {
            const auto& I = rst.getRst<int>("I", 5);
            const auto  expect_I = std::vector<int>{ 1, 2, 3, 4 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 5);
            const auto  expect_L = std::vector<bool> {
                false, false, false, true,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 5);
            const auto  expect_S = std::vector<float>{
                1.23e-04f, 1.234e5f, -5.4321e-9f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 5);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 5);
            const auto  expect_Z = std::vector<std::string>{
                "HELLO", ",", "WORLD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "X0013");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(!rst.hasReportStepNumber( 1));
        BOOST_CHECK(!rst.hasReportStepNumber( 5));
        BOOST_CHECK( rst.hasReportStepNumber(13));

        {
            const auto seqnum        = rst.listOfReportStepNumbers();
            const auto expect_seqnum = std::vector<int>{13};

            BOOST_CHECK_EQUAL_COLLECTIONS(seqnum.begin(), seqnum.end(),
                                          expect_seqnum.begin(),
                                          expect_seqnum.end());
        }

        {
            const auto vectors        = rst.listOfRstArrays(13);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(13);

        {
            const auto& I = rst.getRst<int>("I", 13);
            const auto  expect_I = std::vector<int>{ 35, 51, 13};
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 13);
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 13);
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 13);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 13);
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }
}

BOOST_AUTO_TEST_CASE(Formatted)
{
    const auto rset = RSet("CASE.T01.");
    const auto fmt  = ::Opm::EclIO::OutputStream::Formatted{ true };
    const auto unif = ::Opm::EclIO::OutputStream::Unified  { false };

    {
        const auto seqnum = 1;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 7, 2, 9});
        rst.write("L", std::vector<bool>       {true, false, false, true});
        rst.write("S", std::vector<float>      {3.1f, 4.1f, 59.265f});
        rst.write("D", std::vector<double>     {2.71, 8.21});
        rst.write("Z", std::vector<std::string>{"W1", "W2"});
    }

    {
        const auto seqnum = 13;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {35, 51, 13});
        rst.write("L", std::vector<bool>       {true, true, true, false});
        rst.write("S", std::vector<float>      {17.29e-02f, 1.4142f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180, 123.45e6});
        rst.write("Z", std::vector<std::string>{"G1", "FIELD"});
    }

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0013");

        auto rst = ::Opm::EclIO::ERst{fname};

        {
            const auto vectors        = rst.listOfRstArrays(13);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(13);

        {
            const auto& I = rst.getRst<int>("I", 13);
            const auto  expect_I = std::vector<int>{ 35, 51, 13 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 13);
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 13);
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 13);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 13);
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    {
        // Separate output.  Step 13 should be unaffected.
        const auto seqnum = 5;
        auto rst = ::Opm::EclIO::OutputStream::Restart {
            rset, seqnum, fmt, unif
        };

        rst.write("I", std::vector<int>        {1, 2, 3, 4});
        rst.write("L", std::vector<bool>       {false, false, false, true});
        rst.write("S", std::vector<float>      {1.23e-04f, 1.234e5f, -5.4321e-9f});
        rst.write("D", std::vector<double>     {0.6931, 1.6180});
        rst.write("Z", std::vector<std::string>{"HELLO", ", ", "WORLD"});
    }

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0005");

        auto rst = ::Opm::EclIO::ERst{fname};

        {
            const auto vectors        = rst.listOfRstArrays(5);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 4},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 3},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 2},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 3},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(5);

        {
            const auto& I = rst.getRst<int>("I", 5);
            const auto  expect_I = std::vector<int>{ 1, 2, 3, 4 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.get<bool>("L");
            const auto  expect_L = std::vector<bool> {
                false, false, false, true,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 5);
            const auto  expect_S = std::vector<float>{
                1.23e-04f, 1.234e5f, -5.4321e-9f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 5);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 5);
            const auto  expect_Z = std::vector<std::string>{
                "HELLO", ",", "WORLD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }

    // -------------------------------------------------------
    // Don't rewrite step 13.  Output file should still exist.
    // -------------------------------------------------------

    {
        using ::Opm::EclIO::OutputStream::Restart;

        const auto fname = ::Opm::EclIO::OutputStream::
            outputFileName(rset, "F0013");

        auto rst = ::Opm::EclIO::ERst{fname};

        BOOST_CHECK(rst.hasReportStepNumber(13));

        {
            const auto vectors        = rst.listOfRstArrays(13);
            const auto expect_vectors = std::vector<Opm::EclIO::EclFile::EclEntry>{
                // No SEQNUM in separate output files.
                Opm::EclIO::EclFile::EclEntry{"I", Opm::EclIO::eclArrType::INTE, 3},
                Opm::EclIO::EclFile::EclEntry{"L", Opm::EclIO::eclArrType::LOGI, 4},
                Opm::EclIO::EclFile::EclEntry{"S", Opm::EclIO::eclArrType::REAL, 2},
                Opm::EclIO::EclFile::EclEntry{"D", Opm::EclIO::eclArrType::DOUB, 3},
                Opm::EclIO::EclFile::EclEntry{"Z", Opm::EclIO::eclArrType::CHAR, 2},
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(vectors.begin(), vectors.end(),
                                          expect_vectors.begin(),
                                          expect_vectors.end());
        }

        rst.loadReportStepNumber(13);

        {
            const auto& I = rst.getRst<int>("I", 13);
            const auto  expect_I = std::vector<int>{ 35, 51, 13 };
            BOOST_CHECK_EQUAL_COLLECTIONS(I.begin(), I.end(),
                                          expect_I.begin(),
                                          expect_I.end());
        }

        {
            const auto& L = rst.getRst<bool>("L", 13);
            const auto  expect_L = std::vector<bool> {
                true, true, true, false,
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(L.begin(), L.end(),
                                          expect_L.begin(),
                                          expect_L.end());
        }

        {
            const auto& S = rst.getRst<float>("S", 13);
            const auto  expect_S = std::vector<float>{
                17.29e-02f, 1.4142f,
            };

            check_is_close(S, expect_S);
        }

        {
            const auto& D = rst.getRst<double>("D", 13);
            const auto  expect_D = std::vector<double>{
                0.6931, 1.6180, 123.45e6,
            };

            check_is_close(D, expect_D);
        }

        {
            const auto& Z = rst.getRst<std::string>("Z", 13);
            const auto  expect_Z = std::vector<std::string>{
                "G1", "FIELD",  // ERst trims trailing blanks
            };

            BOOST_CHECK_EQUAL_COLLECTIONS(Z.begin(), Z.end(),
                                          expect_Z.begin(),
                                          expect_Z.end());
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
