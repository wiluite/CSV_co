///
/// \file   test/test.cpp
/// \author wiluite
/// \brief  CSV reader functionality tests.

#define BOOST_UT_DISABLE_MODULE
#include "ut.hpp"
#include <csv_co/reader.hpp>
#include <fstream>

int main() {

    using namespace boost::ut;
    using namespace csv_co;

#if defined (WIN32)
    cfg<override> ={.colors={.none="", .pass="", .fail=""}};
#endif

    "Simple string functions"_test = [] {

        using namespace string_functions;

        cell_string s = "\n\t \r \t\r\n ";
        expect(devastated(s));

        s = R"(""Christmas Tree"" is bad food)";
        unique_quote(s, double_quotes::value);
        expect(s == R"("Christmas Tree" is bad food)");
        auto [a, b] = begins_with(s, '"');
        expect(a);
        expect(b == 0);

        s = "\n\t \r \"";
        auto [c, d] = begins_with(s, '"');
        expect(c);
        expect(d == 5);

        s = "\n\t \r (\"";
        auto [e, f] = begins_with(s, '"');
        expect(!e);
        expect(d == 5);

        s = R"(    "context " )";
        unquote(s, '"');
        expect(s == R"(    context  )");

        s = R"(    "context  )";
        unquote(s, '"');
        expect(s == R"(    "context  )");

        s = R"(    context"  )";
        unquote(s, '"');
        expect(s == R"(    context"  )");

    };

    "Special [del_last] function"_test = [] {

        using namespace string_functions;

        cell_string s = R"(qwerty")";
        expect(del_last(s, '"'));
        expect(s == "qwerty");

        s = "qwerty\"\t\n \r";
        expect(del_last(s, '"'));
        expect(s == "qwerty\t\n \r");

        s = "qwerty\"\t\n~\r";
        expect(!del_last(s, '"'));
        expect(s == "qwerty\"\t\n~\r");

        s = " qwe\"rty\"\t\n~\r";
        expect(!del_last(s, '"'));
        expect(s == " qwe\"rty\"\t\n~\r");

        s = " qwe\"rty\"\t\n\r";
        expect(del_last(s, '"'));
        expect(s == " qwe\"rty\t\n\r");

    };

    "Reader callback calculates cells from char const *"_test = [] {

        auto cells{0u};
        reader r("1,2,3\n4,5,6\n7,8,9\n");

        r.run_spans<1>([&](auto) {
            ++cells;
        });
        expect(cells == 9);

    };

    "Reader span callbacks do not check tabular shape"_test = [] {

        auto cells{0u};
        auto rows {0u};
        reader r("1,2,3\n4,5,6\n7,8\n");

        r.run_spans<1>([&cells](auto a) {
            ++cells;
        }, [&rows]{rows++;});
        expect(cells == 8);
        expect(rows == 3);

    };

    "Reader callback calculates cells from rvalue string"_test = [] {

        auto cells{0u};
        reader r(cell_string("1,2,3\n4,5,6\n"));

        r.run_spans<1>([&](auto) {
            ++cells;
        });
        expect(cells == 6);

    };

    "Reader callbacks calculate cols and rows"_test = [] {

        auto cells{0u}, rows{0u};
        reader r("one,two,three\nfour,five,six\nseven,eight,nine\n");

        r.run_spans<1>([&](auto) {
            cells++;
        }, [&]() {
            rows++;
        });

        expect(cells % rows == 0);
        auto const cols = cells / rows;
        expect(cols == 3 && rows == 3);

    };

    "Reader calculates cols and rows via special methods"_test = [] {

        reader r("one,two,three\nfour,five,six\nseven,eight,nine\n,ten,eleven,twelve\n");
        expect(r.rows<1>() == 4);
        expect(r.cols<1>() == 3);

        // exception, csv-string cannot be empty
        // this is to correspond to behaviour on memory-mapping zero-sized csv-files
        expect(throws([] { reader r2(""); }));

        // NOW YES, there is ONE empty field, so there is a row and a column
        reader r3("\n");
        expect(r3.rows<1>() == 1);
        expect(r3.cols<1>() == 1);

        // as well...
        reader r4(" ");
        expect(r4.rows<1>() == 1);
        expect(r4.cols<1>() == 1);

    };

    "Reader callback is filling data"_test = [] {

        std::vector<cell_string> v;
        std::vector<cell_string> v2{"one", "two", "three", " four", " five", " six", "seven", "eight", "nine"};

        reader r("one,two,three\n four, five, six\nseven,eight,nine\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v2 == v);

    };

    "Reader is trimming data"_test = [] {

        std::vector<cell_string> v;
        std::vector<cell_string> v2{"one", "two", "three", "four", "five", "six", "seven", "eight", "nine"};

        reader<trim_policy::alltrim>
                r("one, \ttwo , three \n four, five, six\n seven , eight\t , nine\r\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v2 == v);

    };

    "Reader [knows] last row may lack line feed"_test = [] {

        std::vector<cell_string> v;
        reader r("one,two,three\nfour,five,six");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v.size() == 6);
        expect(v.back() == "six");
    };

    "Reader with another delimiter character"_test = [] {

        std::vector<cell_string> v;

        using new_delimiter = delimiter<';'>;

        reader<trim_policy::no_trimming, double_quotes, new_delimiter> r("one;two;three\nfour;five;six\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v.size() == 6);
        expect(v == std::vector<cell_string>{{"one", "two", "three", "four", "five", "six"}});

    };

    "Reader provides empty cell as expected"_test = [] {

        std::vector<cell_string> v;
        reader r("one,two,three\nfour,,six\n");

        r.run_spans<1>([&](auto &s) {
            v.emplace_back(s);
        });

        expect(v.size() == 6);
        expect(v == std::vector<cell_string>{{"one", "two", "three", "four", "", "six"}});

    };

    // -- topic change: Quoting --

    // NOTE: ...
    "Incorrect use of single quotes inside quoted cell"_test = [] {

        std::vector<unquoted_cell_string> v;
        std::size_t cells{0u};
        reader r(R"(2022, Mouse, "It's incorrect to use "Hello, Christmas Tree!"" ,, "4900,00")");

        r.run_spans<1>([&](auto &s) {
            cells++;
            v.emplace_back(s);
        });

        //TODO: PROBABLY SOMEHOW SHOULD FIX THIS
        expect(cells == 6);
        expect(v == std::vector<unquoted_cell_string>
                {"2022", " Mouse", R"( "It's incorrect to use "Hello)", R"( Christmas Tree!" )", "", " 4900,00"});

    };

    "Correct use of doubled quotes inside quoted cell"_test = [] {

        std::vector<unquoted_cell_string> v;
        std::size_t cells{0u};
        reader r(R"(2022, Mouse, "It's a correct use case: ""Hello, Christmas Tree!""" ,, "4900,00")");

        r.run_spans<1>([&](auto &s) {
            cells++;
            v.emplace_back(s);
        });

        expect(cells == 5);
        expect(v == std::vector<unquoted_cell_string>
                {{"2022", " Mouse", R"( It's a correct use case: "Hello, Christmas Tree!" )", R"()", R"( 4900,00)"}});

    };

    "Correct use case of quoted parts of the cell"_test = [] {

        std::vector<unquoted_cell_string> v;
        std::size_t cells{0};
        reader r (R"(2022,Mouse,What is quoted is necessary part "Hello, Tree!" of the cell,,"4900,00")");
        r.run_spans<1>([&](auto &s) {
            cells++;
            v.emplace_back(s);
        });

        expect(cells == 5);
        expect(v == std::vector<unquoted_cell_string>
                {{"2022", "Mouse", R"(What is quoted is necessary part "Hello, Tree!" of the cell)", "", "4900,00"}});

    };

    "Reader with another quoting character"_test = [] {

        constexpr std::size_t correct_result = 1;
        auto cells{0u};
        reader r("\"just one, and only one, quoted cell\"\n");

        r.run_spans<1>([&](auto) {
            cells++;
        });
        expect(cells == correct_result);

        constexpr std::size_t incorrect_result = 3;
        cells = 0u;
        reader r2("`just one, and only one, quoted cell`\n");

        r2.run_spans<1>([&](auto) {
            cells++;
        });
        expect(cells == incorrect_result);

        constexpr std::size_t correct_result_again = 1;
        cells = 0u;

        using new_quote_char = quote_char<'`'>;

        reader<trim_policy::no_trimming, new_quote_char> r3("`just one, and only one, quoted cell`\n");

        r3.run_spans<1>([&](auto & s) {
            expect(s == "just one, and only one, quoted cell");
            cells++;
        });
        expect(cells == correct_result_again);

    };

    "bug-fix: LF can be the last char of quoted cell "_test = [] {

        reader r("one,\"quoted, with \r\t\n and last\n\",three");

        r.run_spans<1>([&](auto & s) {
            expect(s == "one" || s == "quoted, with \r\t\n and last\n" || s == "three");
        });

    };

    // -- Topic change: Additional construction --
    "Matrix construction"_test = [] {

        {
            std::vector<std::vector<std::string>> matrix;
            expect(throws([&matrix]() {
                reader r(matrix);
            }));
        }
        {
            std::vector<std::vector<std::string>> matrix ({{"a","\"b\"","c"}, {"d","e","f"}});
            expect(nothrow([&matrix]() {
                reader r(matrix);
            }));
        }

    };

    // -- Topic change: Check Validity --

    "Check validity of a csv source"_test = [] {

        expect(nothrow([]() {
            auto &_ = reader("1,2,3\n").validate<1>();
            (void) _;
        }));
        expect(throws([]() {
            auto &_ = reader("1,2,3\n4\n5,5,5,5").validate<1>();
            (void) _;
        }));
        expect(throws([]() {
            auto &_ = reader("1,2,3\n4,5\n").validate<1>();
            (void) _;
        }));
        expect(nothrow([]() {
            auto &_ = reader("1,2,3\n4,5, 6").validate<1>();
            (void) _;
        }));
        expect(throws([]() {
            auto &_ = reader("1,2,3\n4,5,6,7\n").validate<1>();
            (void) _;
        }));
        expect(throws([]() {
            auto &_ = reader("1,2,3\n4,5,6\n7\n").validate<1>();
            (void) _;
        }));
        expect(nothrow([]() {
            auto &_ = reader("1,2,3\n4,5, 6\n7,8,9").validate<1>();
            (void) _;
        }));
        expect(throws([] {
            auto &_ = reader
                    (std::filesystem::path("game-invalid-format.csv")).validate<1>();
            (void) _;
        }));

        try {
            auto &_ {reader("1,2,3\n4,5\n6,7,8,9\nA,B,C").validate<1>()};
            (void) _;
        } catch(reader<>::exception const & e) {
            expect(std::strcmp(e.what(), R"("Line 2: Expected 3 columns, found 2 columns","Line 3: Expected 3 columns, found 4 columns")") == 0);
        }

        // skip rows
        expect(nothrow([]() {
            auto &_ = reader("Copyright notice\n4,5, 6\n7,8,9").skip_rows(1).validate<1>();
            (void) _;
        }));
        try {
            auto &_ {reader("Copyright notice\n4,5\n7,8,9").skip_rows(1).validate<1>()};
            (void) _;
        } catch(reader<>::exception const & e) {
            expect(std::strcmp(e.what(), R"("Line 2: Expected 2 columns, found 3 columns")") == 0);
        }

    };

    "validated_rows() & validated_cols() methods"_test = [] {

        // exception if called without validate()
        expect(throws([]() {
            auto _ = reader("1,2,3\n4,5,6\n7,8,9\n").validated_rows();
            (void) _;
        }));
        expect(throws([]() {
            auto _ = reader("1,2,3\n4,5,6").validated_cols();
            (void) _;
        }));

        expect(nothrow([]() {
            auto rd = reader("1,2,3\n4,5,6\n");
            auto const & r = rd.validate<1>();

            expect(nothrow([&r]() {
                auto v_rows = r.validated_rows();
                expect(v_rows == 2);
            }));
            expect(nothrow([&r]() {
                auto v_cols = r.validated_cols();
                expect(v_cols == 3);
            }));
        }));
        expect(nothrow([]() {
            auto rd = reader("\"Copyright \"\",\n \"\" notice\"\n1,2,3\n4,5,6\n");
            auto & r = rd.skip_rows(1).validate<1>();
            (void) r;

            expect(nothrow([&r]() {
                auto v_rows = r.validated_rows();
                expect(v_rows == 2);
            }));
            expect(nothrow([&r]() {
                auto v_cols = r.validated_cols();
                expect(v_cols == 3);
            }));
        }));

    };

    // -- Topic change: File processing --

    "Read a well-known file"_test = [] {

        auto cells{0u};
        auto rows{0u};
        unquoted_cell_string first_string;

        expect(nothrow([&] {
                   reader r(std::filesystem::path("game.csv"));
                   r.validate().run_spans<1>([&](auto &s) {
                       cells++;
                       if (rows < 1) {
                           first_string += static_cast<unquoted_cell_string>(s);
                       }
                   }, [&] {
                       rows++;
                   });
                   expect(r.rows<1>() == 14);
                   expect(r.cols<1>() == 6);
               }) >> fatal
        ) << "it shouldn't throw";

        // this depends on line-breaking style, note: reader's trimming  policy is absent.
        expect(first_string == "hello, world1!\r" || first_string == "hello, world1!");

        expect(rows == 14);
        expect(cells / rows == 6);

    };

    "Read an empty file"_test = [] {

        auto cells{0u};
        auto rows{0u};

        std::fstream fs;
        fs.open("empty.csv", std::ios::out);
        fs.close();

        expect(throws([&] {
                   reader r(std::filesystem::path("empty.csv"));
                   r.run_spans<1>([&](auto) {
                       cells++;
                   }, [&] {
                       rows++;
                   });

               }) >> fatal
        ) << "it should throw!";

    };

    "Read a well-known file with header"_test = [] {

        auto cells{0u};
        auto rows{0u};
        std::vector<cell_string> v;
        std::vector<cell_string> v2;

        expect(nothrow([&] {
                   static char const chars[] = "\r";
                   reader<trim_policy::trimming<chars>> r(std::filesystem::path("smallpop.csv"));
                   r.validate().run_spans<1>([&](auto &s) {
                                      v.emplace_back(s);
                                  }, [&](auto &s) {
                                      v2.emplace_back(s);
                                      cells++;
                                  }, [&] {
                                      rows++;
                                  }
                   );

                   expect(rows == r.rows<1>());

               }) >> fatal
        ) << "it shouldn't throw";

        expect(v == std::vector<cell_string>{"city", "region", "country", "population"});
        expect(cells == (rows - 1) * v.size());
        expect(v2.size() == 10 * 4);
        expect(v2.front() == "Southborough");
        expect(v2.back() == "42605");
        expect(nothrow([&v2] { expect(std::stoi(v2.back()) == 42605); }));

    };

    "Read an utf8 file with the first utf8 row and the second usual row"_test = [] {

        auto rows{0u};
        std::vector<unquoted_cell_string> v;
        std::vector<unquoted_cell_string> v2;

        expect(nothrow([&] {
                   static char const chars[] = "\r";
                   reader<trim_policy::trimming<chars>> r(std::filesystem::path("utf8.csv"));
                   r.validate<1>().run_spans<1>([&](auto &s) {
                                      v.emplace_back(s);
                                  }, [&](auto &s) {
                                      v.emplace_back(s);
                                  }, [&] {
                                      rows++;
                                  }
                   );

                   expect(rows == r.rows<1>());
                   expect(v.size() == 8);
                   expect(v[0] == "ЙЦУГ");
                   expect(v[1] == "ГШЩЗ \"ФЫП\" ");
                   expect(v[2] == "ЛДЖЭ");
                   expect(v[3] == "ЯЧИТьбюъ");
                   expect(v[4] == "now");
                   expect(v[5] == " usial");
                   expect(v[6] == "1-byte");
                   expect(v[7] == "encoding");

               }) >> fatal
        ) << "it shouldn't throw";
    };

    "Read an utf8-BOM file from csvkit package"_test = [] {

        auto rows{0u};
        std::vector<cell_string> v;
        std::vector<cell_string> v2;

        expect(nothrow([&] {
                   static char const chars[] = "\r";
                   reader<trim_policy::trimming<chars>> r(std::filesystem::path("test_utf8_bom.csv"));
                   r.validate<1>().run_spans<1>([&v](auto &s) {
                                                    v.emplace_back(s);
                                                }, [&v](auto &s) {
                                                    v.emplace_back(s);
                                                }, [&rows] {
                                                    rows++;
                                                }
                   );

                   expect(rows == r.rows<1>());
                   expect(v.size() == 9);
                   expect(v[0] == "foo");
                   expect(v[1] == "bar");
                   expect(v[2] == "baz");
                   expect(v[3] == "1");
                   expect(v[4] == "2");
                   expect(v[5] == "3");
                   expect(v[6] == "4");
                   expect(v[7] == "5");
                   expect(v[8][0] == static_cast<char>(0xca));
                   expect(v[8][1] == static_cast<char>(0xa4));

               }) >> fatal
        ) << "it shouldn't throw";
    };

    "Read a .gz file"_test = [] {

        std::vector<cell_string> v;
        std::vector<cell_string> v2;

        expect(nothrow([&] {
                   static char const chars[] = "\r";
                   reader<trim_policy::trimming<chars>> r(std::filesystem::path("dummy.csv.gz"));
                   r.run_spans<1>([&](auto &s) {
                                                 v.emplace_back(s);
                                             }, [&](auto &s) {
                                                 v2.emplace_back(s);
                                             }, [&] {
                                             }
                   );
                   expect(v.size()==3 and v2.size()==3);
                   expect(v[0]=="a" && v[1]=="b" && v[2]=="c");
                   expect(v2[0]=="1" && v2[1]=="2" && v2[2]=="3");
               }) >> fatal
        ) << "it shouldn't throw";
    };

    // -- Topic change: Move Operations --

    "Move construction and assignment"_test = [] {

        reader r("One,Two,Three\n1,2,3\n");

        reader r2 = std::move(r);
        //---- r is in Move-From State:
        // But CSV_co is safe in this sense, let us go on.
        expect(throws([&r]() {
            auto &_ = r.validate<1>();
            (void) _;
        }));
        expect(r.cols<1>() == 0);
        expect(r.rows<1>() == 0);
        //-------------------------------------
        expect(nothrow([&r2]() {
            auto &_ = r2.validate<1>();
            (void) _;
        }));

        expect(r2.cols<1>() == 3);
        expect(r2.rows<1>() == 2);

        auto head_cells{0u};
        auto cells{0u};
        auto rows{0u};

        r2.run_spans<1>([&head_cells](auto &s) {
                            expect(s == "One" || s == "Two" || s == "Three");
                            head_cells++;
                        }, [&cells](auto &s) {
                            cell_string cs = s.operator cell_string();
                            expect(cs == "1" || cs == "2" || cs == "3");
                            ++cells;
                        }, [&rows] {
                            rows++;
                        }
        );

        expect(head_cells == 3);
        expect(cells == 3);
        expect(rows == 2);

        // Move back (check for move assignment)
        r = std::move(r2);
        expect(nothrow([&r]() {
            auto &_ = r.validate<1>();
            (void) _;
        }));

        expect(r.cols<1>() == 3);
        expect(r.rows<1>() == 2);

        head_cells = cells = rows = 0;

        r.run_spans<1>([&head_cells](auto &s) {
                           expect(s == "One" || s == "Two" || s == "Three");
                           head_cells++;
                       }, [&cells](auto &s) {
                           expect(s == "1" || s == "2" || s == "3");
                           ++cells;
                       }, [&rows] {
                           rows++;
                       }
        );

        expect(head_cells == 3);
        expect(cells == 3);
        expect(rows == 2);

    };

    "run_spans()'s value callbacks process hard quoted fields"_test = [] {

        {
            auto cells{0u}, rows{0u};
            std::vector<unquoted_cell_string> v;
            reader r(R"( "It's a correct use case: ""Hello, Christmas Tree!""" )");
            r.run_spans<1>([&](auto &s) {
                               v.emplace_back(s);
                               ++cells;
                           }, [&rows] {
                               ++rows;
                           }
            );

            expect(cells == 1);
            expect(rows == 1);
            expect(v == std::vector<unquoted_cell_string>{R"( It's a correct use case: "Hello, Christmas Tree!" )"});
        }

        {
            auto cells{0u}, rows{0u};
            std::vector<cell_string> v;
            reader r("1\n2\n");
            r.run_spans<1>([&](auto & s) {
                               v.push_back(s.operator cell_string());
                               ++cells;
                           }, [&rows] {
                               ++rows;
                           }
            );

            expect(cells == 2);
            expect(rows == 2);
            expect(v == std::vector<cell_string>{"1","2"});
        }

        {
            std::vector<cell_string> v;
            reader r(R"( "quoted from the beginning, only" with usual rest part)");
            r.run_spans<1>([&](auto & s) {
                               v.push_back(s.operator cell_string());
                           }
            );
            expect(v == std::vector<cell_string>{R"( "quoted from the beginning, only" with usual rest part)"});

        }

        {
            std::vector<unquoted_cell_string> v;
            reader r(R"( " quoted from the beginning, only (with inner ""a , b"") " with usual rest part)");
            r.run_spans<1>([&](auto & s) {
                               v.push_back(s.operator unquoted_cell_string());
                           }
            );
            expect(v == std::vector<unquoted_cell_string>
                    {R"( " quoted from the beginning, only (with inner "a , b") " with usual rest part)"});

        }

        {
            std::vector<unquoted_cell_string> v;
            reader r(R"( quoted in the "middle, only (with inner ""a , b"") " with usual rest part)");
            r.run_spans<1>([&](auto & s) {
                               v.push_back(s.operator unquoted_cell_string());
                           }
            );
            expect(v == std::vector<unquoted_cell_string>
                    {R"( quoted in the "middle, only (with inner "a , b") " with usual rest part)"});

        }

    };

    "run_spans()'s header and value callbacks process hard quoted fields"_test = []{

        {
            auto h_cells{0u}, rows{0u};
            auto v_cells{0u};
            std::vector<unquoted_cell_string> v;
            std::vector<unquoted_cell_string> v2;
            reader r(R"( "It's a correct use case: ""Hello, Christmas Tree!"""
"It's a correct use case: ""Hello, Christmas Tree!""" )");
            r.run_spans<1>([&h_cells, &v](auto &s) {
                               v.push_back(s.operator unquoted_cell_string());
                               ++h_cells;
                           }, [&v_cells, &v2](auto &s) {
                               v2.push_back(s.operator unquoted_cell_string());
                               ++v_cells;
                           }, [&rows] {
                               ++rows;
                           }
            );

            expect(h_cells == 1);
            expect(rows == 2);
            expect(v_cells == 1);
            expect(v == std::vector<unquoted_cell_string>{R"( It's a correct use case: "Hello, Christmas Tree!")"});
            expect(v2 == std::vector<unquoted_cell_string>{R"(It's a correct use case: "Hello, Christmas Tree!" )"});
        }

    };

    // -- so now... topic change: lexical casts and comparisons between numeric types and cells' contents --
    "lexical casts"_test = [] {

        cell_string cs;
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&s] { s.as<int>(); }));
            try { s.as<int>();}
            catch(reader<>::exception const & ex) { expect (std::strcmp(ex.what(), "Argument is empty") == 0);}
            expect (s.operator cell_string().empty());

            reader<>::cell_span ss (cs);
            expect(throws([&ss] { ss.as<int>(); }));
            try { ss.as<int>();}
            catch(reader<>::exception const & ex) { expect (std::strcmp(ex.what(), "Argument is empty") == 0);}
            expect (ss.operator cell_string().empty());

            cs = " ";
            expect(throws([&cs]{ reader<>::cell_span s (cs.begin(), cs.end());s.as<int>(); }));
            try {
                reader<>::cell_span _ (cs.begin(), cs.end()); _.as<int>();
            } catch(reader<>::exception const & ex) {
                expect(std::strcmp(ex.what(), "Argument is empty") == 0);
            }
            reader<>::cell_span sss (cs);
            expect (sss.operator cell_string() == " ");
        }

        // check signed conversions and boundaries
        cs = "   -2147483647a ";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(nothrow([&s] {
                const auto value = s.as<int>(); expect(value == -2147483647);
            }));

            expect(s == -2147483647);
        }

        cs = " 2147483648a";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&s] { s.as<int>(); }));
            try { s.as<int>();}
            catch(reader<>::exception const & ex) {
                expect (std::strcmp(ex.what(), "This number  2147483648a is larger than int") == 0);
            }
        }

        // check unsigned conversions and boundaries
        cs = "65535";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            unsigned short value{};
            expect(nothrow([&] { value = s.as<decltype(value)>(); }));
            expect(value == 65535);
            expect(s == 65535);
        }

        cs = " 65536";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&s] { s.as<unsigned short>(); }));
        }

        // check for not number
        cs = "not a number";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&s] { s.as<int>(); }));
            try { s.as<int>();}
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "Argument isn't a number: not a number") == 0);}
        }

        // check for floating-point values

        // float
        cs = " 42.5555554321f   ";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            float value{};
            expect(nothrow([&] { value = s.as<float>(); }));
            expect(value == 42.5555554f);
            expect(s == 42.5555554f);
        }

        cs = " 0";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            float value{};
            expect(nothrow([&] { value = s.as<float>(); }));
            expect(value == 0.0f);
            expect(s == 0.0f);
        }

        cs = "NaN";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            // NAN != NAN by Standard
            expect(s != NAN);
            float value{};
            expect(nothrow([&] { value = s.as<float>(); }));
            expect(std::isnan(value));
        }

        cs = " iNf";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(s == +INFINITY);
            long double value{};
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(std::isinf(value));
        }

        cs = " -iNf";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(s == -INFINITY);
            long double value{};
            expect(nothrow([&] { value = s.as<float>(); }));
            expect(std::isinf(value));
            expect(nothrow([&] { value = s.as<double>(); }));
            expect(std::isinf(value));
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(std::isinf(value));
        }

        cs = " r";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<float>(); }));
            try { s.as<float>(); }
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "Argument isn't a number:  r") == 0);}
        }

        cs = " 3.40283e+38";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<float>(); }));
            try { s.as<float>(); }
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "This number  3.40283e+38 is larger than float") == 0);}
        }

        // double
        cs = "42.55555543211";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            double value{};
            expect(nothrow([&] { value = s.as<double>(); }));
            expect(value == 42.55555543211);
            expect(s == 42.55555543211);
        }

        cs = "0";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            double value{};
            expect(nothrow([&] { value = s.as<double>(); }));
            expect(value == 0.0);
            expect(s == 0.0);
        }

        cs = "Nan";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            double value{};
            expect(nothrow([&] { value = s.as<double>(); }));
            expect(std::isnan(value));
            // NAN != NAN by Standard
            expect(s != NAN);
        }

        cs = "iNf";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            double value{};
            expect(nothrow([&] { value = s.as<double>(); }));
            expect(std::isinf(value));
            expect(s == INFINITY);
        }

        cs = " r";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<double>(); }));
            try { s.as<double>(); }
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "Argument isn't a number:  r") == 0);}
        }

        cs = " 1.79769e+309";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<double>(); }));
            try { s.as<double>(); }
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "This number  1.79769e+309 is larger than double") == 0);}
        }

        // long double
        cs = " 42.55555567";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            long double value{};
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(std::abs(value - 42.55555567) <= 0.00000000000001);
            expect(std::abs(s - 42.55555567) <= 0.00000000000001);
        }

        cs = "0";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            long double value{};
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(value == 0.0);
            expect(s == 0.0);
        }

        cs = "Nan";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            long double value{};
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(std::isnan(value));
            // NAN != NAN by Standard
            expect(s != NAN);
        }

        cs = "iNf";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            long double value{};
            expect(nothrow([&] { value = s.as<long double>(); }));
            expect(std::isinf(value));
            expect(s == INFINITY);
        }

        cs = " r";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<long double>(); }));
            try { s.as<long double>(); }
            catch(reader<>::exception const & ex)
            { expect (std::strcmp(ex.what(), "Argument isn't a number:  r") == 0);}
        }

        cs = " 1.18974e+4932";
        {
            reader<>::cell_span s (cs.begin(), cs.end());
            expect(throws([&] { s.as<long double>(); }));
            try { s.as<long double>(); }
            catch(reader<>::exception const & ex)
#ifdef _MSC_VER
            { expect (std::strcmp(ex.what(), "This number  1.18974e+4932 is larger than long double") == 0);}
#else
            { expect (std::strcmp(ex.what(), "Range error, got inf:  1.18974e+4932") == 0);}
#endif
        }

    };

    // -- so now... topic change: row callbacks ---

    "run_rows()'s callback with no more data throws"_test = [] {

        auto cells{0u};
        reader r(" \n");
        r.skip_rows(1);
        expect(r.cols() == 0);
        try {
            r.run_rows<1>([&](auto){
            });
        } catch (typename reader<>::implementation_exception const & e) {
            expect(std::strcmp(e.what(),"An incorrect assumption, columns number is zero.") == 0);
        }
    };

    "run_rows()' value callbacks"_test = [] {

        reader r ("aa,bbb,cccc,ddddd\n4444,555,66,6\n\"0,14\",\"15.0\",\"16,67\",\n");
        auto current_row = 0;
        r.validate<1>().run_rows<1>([&](auto & row){

            if (current_row == 0) {
                expect (row[0]=="aa");
                expect (row[0]!="aaa");
                expect (row[1]=="bbb");
                expect (row[2]=="cccc");
                expect (row[3]=="ddddd");
            } else if (current_row == 1) {
                expect (row[0]=="4444");
                expect (row[1]=="555");
                expect (row[2]=="66");
                expect (row[3]=="6");
            } else {
                auto const cs = static_cast<cell_string>(row.front());
                expect (row.front() == cell_string{"0,14"});
                expect (row.front() == "0,14");
                expect ("0,14" == row.front());
                expect (cell_string{"0,14"} == row.front());
                expect (cs == cell_string{"\"0,14\""});
                expect (row[1]=="15.0");
                expect (row[2]=="16,67");
                expect (row.back()=="");
            }
            ++current_row;

        });
        expect (current_row == 3);

    };

    "run_rows()' header and value callbacks"_test = [] {

        auto current_row = 0;
        reader("header_field1,header_field2,header_field3,header_field4\n"
               "4444,555,66,6\n\"0,14\",\"15.0\",\"16.67890\",\n").
                validate<1>().run_rows<1>(
                [&](auto & row) {
                    expect (row.front()=="header_field1");
                    expect (row[1]=="header_field2");
                    expect (row[2]=="header_field3");
                    expect (row.back()=="header_field4");
                    ++current_row;
                },
                [&](auto & row){
                    if (current_row == 1) {
                        expect (row[0]=="4444");
                        expect (row["header_field1"]=="4444");
                        expect (row["header_field1"]==4444);
                        expect (row[1]=="555");
                        expect (row["header_field2"]=="555");
                        expect (row["header_field2"]== 555);
                        expect (row[2]=="66");
                        expect (row["header_field3"]=="66");
                        expect (row["header_field3"]== 66);
                        expect (row[3]=="6");
                        expect (row["header_field4"]=="6");
                        expect (row["header_field4"]== 6);
                    } else {
                        unquoted_cell_string cs = row.front().operator unquoted_cell_string();
                        expect (row.front() == "0,14");
                        expect ("0,14" == row.front());
                        expect (cell_string{"0,14"} == row.front());
                        expect (cs == cell_string{"0,14"});
                        expect (row[1]=="15.0");
                        expect (row["header_field2"]=="15.0");
                        expect (row["header_field2"]== 15.0);
                        expect (row[2]=="16.67890");
                        expect (row["header_field3"]=="16.67890");
                        expect (row["header_field3"]== 16.6789);
                        expect (row.back()=="");
                        expect (row.size()==4);
                    }
                    ++current_row;
                }
        );
        expect (current_row == 3);

    };

    "run_rows()' iteration inside callbacks"_test = [] {

        auto total_cells = 0;
        reader("header_field1,header_field2,header_field3,header_field4\n"
               "4444,555,66,6\n\"0,14\",\"15.0\",\"16.67890\",\n").
                validate<1>().run_rows<1>(
                [&](auto & row) {
                    for (auto & e : row) {
                        (void)e;
                        total_cells++;
                    }
                },
                [&](auto & row){
                    for (auto & e : row) {
                        (void)e;
                        total_cells++;
                    }
                }
        );
        expect (total_cells == 12);

    };

    "cell_span to raw string and string_view conversion"_test = [] {

        auto cs = R"(""Christmas Tree"" is bad food)";
        {
            reader<>::cell_span s (cs, cs + std::strlen(cs));
            expect(nothrow([&s] {
                unquoted_cell_string value = s.operator unquoted_cell_string();
                expect(value == R"("Christmas Tree" is bad food)");

                auto value_ = s.raw_string();
                expect(value_ ==  R"(""Christmas Tree"" is bad food)");

                auto sv2 = s.raw_string_view();
                expect(sv2 ==  R"(""Christmas Tree"" is bad food)");
            }));
        }

    };

    "fired installed notification handler with max_field_size_reason notification reason"_test = [] {

        using reader_type = reader<trim_policy::no_trimming, double_quotes, comma_delimiter, non_mac_ln_brk, MFS::trace<5>>;

        size_t notification_handler_fired = 0;
        {
            reader_type r ("1234,12345,777\n");
            r.install_notification_handler([&notification_handler_fired](auto ) {
                notification_handler_fired++;
            });

            expect((r.run_rows<1>([](auto) {}), notification_handler_fired == 0));
            expect((r.run_spans<1>([](auto) {}), notification_handler_fired == 0));
            [[maybe_unused]] auto & _ = r.validate();
            expect((notification_handler_fired == 0));
            [[maybe_unused]] auto c=r.cols();
            expect((notification_handler_fired == 0));
            [[maybe_unused]] auto w =r.rows();
            expect((notification_handler_fired == 0));
        }

        try {
            reader_type r ("12345,123456,777\n");
            r.install_notification_handler([&notification_handler_fired](auto s) {
                notification_handler_fired++;
                if (reader_type::notification_reason::max_field_size_reason == s) {
                    throw reader_type::exception("max_length");
                }
            });
            r.run_rows<1>([](auto) {});
        } catch(reader_type::exception const &) {
            notification_handler_fired++;
        }
        expect(notification_handler_fired == 2);

        notification_handler_fired = 0;
        {
            reader_type r ("12345,123456,777\n");
            r.install_notification_handler([&notification_handler_fired](auto ) {
                notification_handler_fired++;
            });
            r.run_spans<1>([](auto) {});
            expect(notification_handler_fired == 1);

            [[maybe_unused]] auto & _ = r.validate();
            expect((notification_handler_fired == 2));
            [[maybe_unused]] auto c = r.cols();
            expect((notification_handler_fired == 3));
            [[maybe_unused]] auto w = r.rows();
            expect((notification_handler_fired == 4));
        }
        // for double quoted cells
        {
            notification_handler_fired = 0;
            using reader_type2 = reader<trim_policy::no_trimming, double_quotes, comma_delimiter, non_mac_ln_brk, MFS::trace<>>;

            reader_type2 r (R"("1234     5678")");
            r.update_limit(15);
            r.install_notification_handler([&notification_handler_fired](auto) {
                notification_handler_fired++;
            });
            r.run_rows<1>([](auto) {});
            expect(notification_handler_fired == 0);

            reader_type2 r2 (R"("1234"",""5678")");
            r2.update_limit(15);
            r2.install_notification_handler([&notification_handler_fired](auto) {
                notification_handler_fired++;
            });
            r2.run_rows<1>([](auto) {});
            expect(notification_handler_fired == 0);

            reader_type2 r3 (R"("1234     56789")");
            r3.update_limit(15);
            r3.install_notification_handler([&notification_handler_fired](auto) {
                notification_handler_fired++;
            });
            r3.run_rows<1>([](auto) {});
            expect(notification_handler_fired == 2); // +1 (and +1 for cols() inside run_row())
            [[maybe_unused]] auto & _ = r3.validate();
            expect((notification_handler_fired == 3));

            reader_type2 r4 (R"("1234"",""56789")");
            r4.update_limit(15);
            r4.install_notification_handler([&notification_handler_fired](auto) {
                notification_handler_fired++;
            });
            [[maybe_unused]] auto & v_ = r4.validate();
            expect((notification_handler_fired == 4));
        }

    };

    "skip_rows"_test = [] {

        using reader_type = reader<>;
        std::vector<cell_string> v;
        auto rows {0u};
        reader_type("\xEF\xBB\xBF Copyright notice,and other\n111,222,333").skip_rows<1>(1).
                run_spans([&v](auto & value){
                              v.push_back(value.operator cell_string());
                          }
                , [&rows](){++rows;});
        expect(v[0]=="111");
        expect(v[1]=="222");
        expect(v[2]=="333");

    };

    "ignore empty rows"_test = [] {

        using reader_type = reader<trim_policy::alltrim, double_quotes, comma_delimiter, non_mac_ln_brk, MFS::no_trace, ER::ignore>;
        {
            std::vector<cell_string> v;
            auto rows{0u};
            reader_type r ("\xEF\xBB\xBF Copyright notice,and other\r\n111,333\r\n\r\n\r\n444,66_\r\n");
            r.skip_rows<1>(1)
            .validate<1>()
            .run_spans([&v](auto &value) { v.push_back(value.operator cell_string()); },[&rows]() { ++rows; });
            expect(v[0] == "111");
            expect(v[1] == "333");
            expect(v[2] == "444");
            expect(v[3] == "66_");
            expect (rows == 2);
            expect (r.validated_rows()==2);
            expect (r.validated_cols()==2);
        }
        {
            std::vector<cell_string> v;
            auto rows{0u};
            reader_type("\xEF\xBB\xBF Copyright notice and other\n111,333\n\n\n444,66_")
                    .skip_rows<1>(1)
                    .run_spans([&v](auto &value) { v.push_back(value.operator cell_string()); },
                               [&rows]() { ++rows; });
            expect(v[0] == "111");
            expect(v[1] == "333");
            expect(v[2] == "444");
            expect(v[3] == "66_");
            expect (rows == 2);
        }
        {
            bool notification_handler_fired = false;
            reader_type("\xEF\xBB\xBF Copyright notice and other\n111,333\n\n\n444,66_")
                    .skip_rows<1>(1)
                    .install_notification_handler([&](auto reason) {
                        notification_handler_fired = true;
                        expect(reason == reader_type::notification_reason::empty_rows_reason);
                    })
                    .run_rows([](auto & row) {
                        expect(row[0] == 111 || row[0]==444);
                        expect(row[1] == 333 || row[1]=="66_");
                    });
            expect(notification_handler_fired);
        }
    };

    "seek"_test = [] {
        {
            reader r("\n");
            r.seek<1>();
            auto header = r.header();
            expect(header.empty());
        }
        {
            reader r("1,2,3");
            r.seek<1>();
            auto header = r.header();
            expect(header.size()==3);
        }
        {
            reader<trim_policy::alltrim> r("\r\n\r\n1,2,3");
            r.seek<1>();
            auto header = r.header();
            expect(header.size()==3);
        }
        {
            reader<trim_policy::alltrim> r("\xEF\xBB\xBF\r\n\r\n\n1,2,3");
            r.seek<1>();
            auto header = r.header();
            expect(header[0]=="1" and header[1]=="2" and header[2]=="3");
        }
    };

}

