#pragma once

namespace csvkit::cli::SOCI {
    enum struct backend_id {
        PG,
        ORCL,
        FB,
        ANOTHER
    };
}
