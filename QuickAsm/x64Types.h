#pragma once

union CR0 {
    size_t All;
    struct {
        unsigned PE : 1;          //!< [0] Protected Mode Enabled
        unsigned MP : 1;          //!< [1] Monitor Coprocessor FLAG
        unsigned EM : 1;          //!< [2] Emulate FLAG
        unsigned TS : 1;          //!< [3] Task Switched FLAG
        unsigned ET : 1;          //!< [4] Extension Type FLAG
        unsigned NE : 1;          //!< [5] Numeric Error
        unsigned reserved1 : 10;  //!< [6:15]
        unsigned WP : 1;          //!< [16] Write Protect
        unsigned reserved2 : 1;   //!< [17]
        unsigned AM : 1;          //!< [18] Alignment Mask
        unsigned reserved3 : 10;  //!< [19:28]
        unsigned NW : 1;          //!< [29] Not Write-Through
        unsigned CD : 1;          //!< [30] Cache Disable
        unsigned PG : 1;          //!< [31] Paging Enabled
    };
};

