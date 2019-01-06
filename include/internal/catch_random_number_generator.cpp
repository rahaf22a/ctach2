/*
 *  Created by Martin on 30/08/2017.
 *
 *  Distributed under the Boost Software License, Version 1.0. (See accompanying
 *  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "catch_random_number_generator.h"
#include "catch_context.h"
#include "catch_interfaces_config.h"

namespace Catch {

    std::mt19937& rng() {
        thread_local static std::mt19937 s_rng;
        return s_rng;
    }

    void seedRng( IConfig const& config ) {
        if( config.rngSeed() != 0 ) {
            rng().seed( config.rngSeed() );
        }
    }

    std::mt19937::result_type rngSeed() {
        return getCurrentContext().getConfig()->rngSeed();
    }
}
