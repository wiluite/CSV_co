#pragma once

namespace {
    static bool is1904;

    inline static std::chrono::system_clock::time_point to_chrono_time_point(double d) {
        using namespace std::chrono;
        using namespace date;
        using ddays = duration<double, date::days::period>;
        if (is1904)
            return date::sys_days{date::January/01/1904} + round<system_clock::duration>(ddays{d});
        else if (d < 60)
            return date::sys_days{date::December/31/1899} + round<system_clock::duration>(ddays{d});
        else
            return date::sys_days{date::December/30/1899} + round<system_clock::duration>(ddays{d});
    }
}

