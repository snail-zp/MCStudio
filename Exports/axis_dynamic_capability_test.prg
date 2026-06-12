!
! Axis dynamic capability test
! Usage:
! 1. Set DYNCAP_AXIS to the target axis number.
! 2. Set DYNCAP_MAX_VEL / DYNCAP_MAX_ACC / DYNCAP_MAX_JERK.
! 3. The move range defaults to 1 mm inward from current soft limits.
! 4. Run AXIS_DYNAMIC_CAPABILITY_TEST.
!
! Result:
! - DYNCAP_START_POS(move)            : move start position
! - DYNCAP_END_POS(move)              : move end position
! - DYNCAP_MOVE_DIRECTION(move)       : 1=negative to positive, -1=positive to negative
! - DYNCAP_CMD_VEL(move)              : commanded velocity
! - DYNCAP_CMD_ACC(move)              : commanded acceleration
! - DYNCAP_CMD_JERK(move)             : commanded jerk
! - DYNCAP_VEL_DATA(move)(sample)     : sampled velocity
!

INT BUFFER_ID; BUFFER_ID = 35

GLOBAL INT DYNCAP_AXIS
GLOBAL INT DYNCAP_DONE
GLOBAL INT DYNCAP_ERROR
GLOBAL INT DYNCAP_CUR_MOVE
GLOBAL INT DYNCAP_SAMPLE_COUNT
GLOBAL INT DYNCAP_MOVE_COUNT

GLOBAL REAL DYNCAP_LEFT_LIMIT
GLOBAL REAL DYNCAP_RIGHT_LIMIT
GLOBAL REAL DYNCAP_MAX_VEL
GLOBAL REAL DYNCAP_MAX_ACC
GLOBAL REAL DYNCAP_MAX_JERK
GLOBAL REAL DYNCAP_SAMPLE_INTERVAL_MS
GLOBAL REAL DYNCAP_MOVE_TIMEOUT_MS
GLOBAL REAL DYNCAP_ENABLE_TIMEOUT_MS
GLOBAL REAL DYNCAP_EDGE_MARGIN_MM
GLOBAL REAL DYNCAP_SETTLE_MS

GLOBAL REAL DYNCAP_START_POS(2)
GLOBAL REAL DYNCAP_END_POS(2)
GLOBAL REAL DYNCAP_CMD_VEL(2)
GLOBAL REAL DYNCAP_CMD_ACC(2)
GLOBAL REAL DYNCAP_CMD_JERK(2)
GLOBAL INT DYNCAP_MOVE_DIRECTION(2)
GLOBAL REAL DYNCAP_VEL_DATA(2)(10000)
GLOBAL REAL DYNCAP_DC_DATA(1)(10000)

AXIS_DYNAMIC_CAPABILITY_TEST:
    LOCAL INT axis_no
    LOCAL INT move_index
    LOCAL INT j
    LOCAL REAL start_pos
    LOCAL REAL end_pos
    LOCAL REAL dc_timeout_ms

    DYNCAP_DONE = 0
    DYNCAP_ERROR = 0
    DYNCAP_CUR_MOVE = -1
    DYNCAP_MOVE_COUNT = 0

    axis_no = DYNCAP_AXIS

    if DYNCAP_SAMPLE_COUNT <= 0
        DYNCAP_SAMPLE_COUNT = 5000
    end

    if DYNCAP_SAMPLE_INTERVAL_MS <= 0
        DYNCAP_SAMPLE_INTERVAL_MS = 1
    end

    if DYNCAP_MOVE_TIMEOUT_MS <= 0
        DYNCAP_MOVE_TIMEOUT_MS = 120000
    end

    if DYNCAP_ENABLE_TIMEOUT_MS <= 0
        DYNCAP_ENABLE_TIMEOUT_MS = 5000
    end

    if DYNCAP_EDGE_MARGIN_MM <= 0
        DYNCAP_EDGE_MARGIN_MM = 1
    end

    if DYNCAP_SETTLE_MS <= 0
        DYNCAP_SETTLE_MS = 100
    end

    if DYNCAP_SAMPLE_COUNT > 10000
        DYNCAP_ERROR = 101
        goto test_error
    end

    if DYNCAP_MAX_VEL <= 0
        DYNCAP_ERROR = 102
        goto test_error
    end

    if DYNCAP_MAX_ACC <= 0
        DYNCAP_ERROR = 103
        goto test_error
    end

    if DYNCAP_MAX_JERK <= 0
        DYNCAP_ERROR = 104
        goto test_error
    end

    DYNCAP_LEFT_LIMIT = SLLIMIT(axis_no)
    DYNCAP_RIGHT_LIMIT = SRLIMIT(axis_no)

    if DYNCAP_RIGHT_LIMIT <= DYNCAP_LEFT_LIMIT
        DYNCAP_ERROR = 105
        goto test_error
    end

    start_pos = DYNCAP_LEFT_LIMIT + DYNCAP_EDGE_MARGIN_MM
    end_pos = DYNCAP_RIGHT_LIMIT - DYNCAP_EDGE_MARGIN_MM
    if end_pos <= start_pos
        DYNCAP_ERROR = 106
        goto test_error
    end

    dc_timeout_ms = (DYNCAP_SAMPLE_COUNT * DYNCAP_SAMPLE_INTERVAL_MS) + 2000

    FCLEAR axis_no
    ENABLE axis_no
    TILL MST(axis_no).#ENABLED, DYNCAP_ENABLE_TIMEOUT_MS
    if ^MST(axis_no).#ENABLED
        DYNCAP_ERROR = 108
        goto test_error
    end

    VEL(axis_no) = DYNCAP_MAX_VEL
    ACC(axis_no) = DYNCAP_MAX_ACC
    DEC(axis_no) = DYNCAP_MAX_ACC
    KDEC(axis_no) = DYNCAP_MAX_ACC
    JERK(axis_no) = DYNCAP_MAX_JERK

    move_index = 0

    PTP axis_no, start_pos
    TILL ^MST(axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(axis_no).#MOVE
        DYNCAP_ERROR = 201
        goto test_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = move_index
    DYNCAP_START_POS(move_index) = start_pos
    DYNCAP_END_POS(move_index) = end_pos
    DYNCAP_CMD_VEL(move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(move_index) = 1

    FILL(0, DYNCAP_DC_DATA)
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(axis_no)
    PTP axis_no, end_pos
    j = 0
    TILL ^S_ST.#DC, dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 205
        goto test_error
    end
    TILL ^MST(axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(axis_no).#MOVE
        DYNCAP_ERROR = 202
        goto test_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(move_index)(j) = DYNCAP_DC_DATA(0)(j)
        j = j + 1
    END
    move_index = move_index + 1

    PTP axis_no, end_pos
    TILL ^MST(axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(axis_no).#MOVE
        DYNCAP_ERROR = 203
        goto test_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = move_index
    DYNCAP_START_POS(move_index) = end_pos
    DYNCAP_END_POS(move_index) = start_pos
    DYNCAP_CMD_VEL(move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(move_index) = -1

    FILL(0, DYNCAP_DC_DATA)
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(axis_no)
    PTP axis_no, start_pos
    j = 0
    TILL ^S_ST.#DC, dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 206
        goto test_error
    end
    TILL ^MST(axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(axis_no).#MOVE
        DYNCAP_ERROR = 204
        goto test_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(move_index)(j) = DYNCAP_DC_DATA(0)(j)
        j = j + 1
    END
    move_index = move_index + 1

    DYNCAP_MOVE_COUNT = move_index
    DYNCAP_DONE = 1
    disp "AXIS %d DYNAMIC CAPABILITY DONE, moves=%d, samples=%d", axis_no, DYNCAP_MOVE_COUNT, DYNCAP_SAMPLE_COUNT
STOP

test_error:
    STOPDC
    HALT axis_no
    disp "AXIS %d DYNAMIC CAPABILITY FAILED, error=%d, move=%d", axis_no, DYNCAP_ERROR, DYNCAP_CUR_MOVE
STOP
