#pragma once

#define Z_CHECK(CALLFUNC, READER, SKIP_LINES, NO_HEADER, MAX_FIELD_SIZE, CALLRESULT) \
    args.skip_lines = static_cast<int>(SKIP_LINES);                                  \
    args.no_header = static_cast<bool>(NO_HEADER);                                   \
    args.maxfieldsize = MAX_FIELD_SIZE;                                              \
    bool thrown##READER = false;                                                     \
    try {                                                                            \
        READER rdr(args.file);                                                       \
        std::reference_wrapper<READER> rdr_ref = std::ref(rdr);                      \
        std::stringstream cout_buffer;                                               \
        {                                                                            \
            redirect(cout)                                                           \
            redirect_cout cr(cout_buffer.rdbuf());                                   \
            CALLFUNC(rdr_ref, args);                                                 \
        }                                                                            \
    } catch(std::runtime_error const & e) {                                          \
        thrown##READER = true;                                                       \
        expect(std::string(e.what()) == CALLRESULT);                                 \
    }                                                                                \
    expect(thrown##READER);

#define Z_CHECK0(CALLFUNC, idx, SKIP_LINES, NO_HEADER, MAX_FIELD_SIZE, CALLRESULT) \
    args_copy.skip_lines = static_cast<int>(SKIP_LINES);                           \
    args_copy.no_header = static_cast<bool>(NO_HEADER);                            \
    args_copy.maxfieldsize = MAX_FIELD_SIZE;                                       \
    bool thrown##idx = false;                                                      \
    try {                                                                          \
        std::stringstream cout_buffer;                                             \
        {                                                                          \
            redirect(cout)                                                         \
            redirect_cout cr(cout_buffer.rdbuf());                                 \
            CALLFUNC;                                                              \
        }                                                                          \
        expect(cout_buffer.str() == CALLRESULT);                                   \
    } catch(std::runtime_error const & e) {                                        \
        thrown##idx = true;                                                        \
        expect(std::string(e.what()) == CALLRESULT);                               \
    }                                                                              \
    expect(thrown##idx);

#define Z_CHECK1(CALLFUNC, idx, SKIP_LINES, NO_HEADER, MAX_FIELD_SIZE, CALLRESULT) \
    args.skip_lines = static_cast<int>(SKIP_LINES);                                \
    args.no_header = static_cast<bool>(NO_HEADER);                                 \
    args.maxfieldsize = MAX_FIELD_SIZE;                                            \
    bool thrown##idx = false;                                                      \
    try {                                                                          \
        CALLFUNC;                                                                  \
    } catch(std::runtime_error const & e) {                                        \
        thrown##idx = true;                                                        \
        expect(std::string(e.what()) == CALLRESULT);                               \
    }                                                                              \
    expect(thrown##idx);

#define Z_CHECK2(READER, SKIP_LINES, NO_HEADER, MAX_FIELD_SIZE, CALLRESULT)  \
    args.skip_lines = static_cast<int>(SKIP_LINES);                          \
    args.no_header = static_cast<bool>(NO_HEADER);                           \
    args.maxfieldsize = MAX_FIELD_SIZE;                                      \
    bool thrown##READER = false;                                             \
    try {                                                                    \
        READER rdr(args.file);                                               \
        std::reference_wrapper<READER> ref = std::ref(rdr);                  \
        csvstat::stat(ref, args);                                            \
    } catch(std::runtime_error const & e) {                                  \
        thrown##READER = true;                                               \
        expect(std::string(e.what()) == CALLRESULT);                         \
    }                                                                        \
    expect(thrown##READER);

namespace z_test {
    enum class skip_lines {
        skip_lines_0,
        skip_lines_1
    };
    enum class header {
        has_header,
        no_header
    };
}
