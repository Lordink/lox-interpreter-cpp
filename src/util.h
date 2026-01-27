#pragma once

// Early return err if this expected stores error
#define UNWRAP(val)                                                            \
    if (!val) {                                                                \
        return val;                                                            \
    }
