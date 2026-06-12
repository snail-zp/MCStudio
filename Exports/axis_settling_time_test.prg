!
! Axis settling time test
! Usage:
! 1. Set SETTLE_AXIS to the target axis number.
! 2. The test range defaults to current SLLIMIT/SRLIMIT.
! 3. Set SETTLE_STEP_DISTANCE / SETTLE_SAMPLE_COUNT / SETTLE_SAMPLE_INTERVAL_MS as needed.
! 4. Run AXIS_SETTLING_TIME_TEST.
!
! Result:
! - SETTLE_POINT_POS(i)              : stepped positions from negative limit to positive limit
! - SETTLE_MOVE_TARGET(move)         : target position for each move
! - SETTLE_MOVE_DIRECTION(move)      : 1=forward, -1=backward
! - SETTLE_POS_DATA(move)(sample)    : position samples from move start
! - SETTLE_AST_INPOS(move)(sample)   : AST(axis).#INPOS samples from move start
! - SETTLE_MST_INPOS(move)(sample)   : MST(axis).#INPOS samples from move start
!

INT BUFFER_ID; BUFFER_ID = 33

GLOBAL INT SETTLE_AXIS
GLOBAL INT SETTLE_DONE
GLOBAL INT SETTLE_ERROR
GLOBAL INT SETTLE_CUR_MOVE
GLOBAL INT SETTLE_POINT_COUNT
GLOBAL INT SETTLE_SAMPLE_COUNT
GLOBAL INT SETTLE_MOVE_COUNT

GLOBAL REAL SETTLE_LEFT_LIMIT
GLOBAL REAL SETTLE_RIGHT_LIMIT
GLOBAL REAL SETTLE_STEP_DISTANCE
GLOBAL REAL SETTLE_SAMPLE_INTERVAL_MS
GLOBAL REAL SETTLE_MOVE_TIMEOUT_MS
GLOBAL REAL SETTLE_ENABLE_TIMEOUT_MS
GLOBAL REAL SETTLE_POINT_POS(100)
GLOBAL REAL SETTLE_MOVE_TARGET(200)
GLOBAL REAL SETTLE_POS_DATA(200)(500)
GLOBAL INT SETTLE_MOVE_DIRECTION(200)
GLOBAL INT SETTLE_AST_INPOS(200)(500)
GLOBAL INT SETTLE_MST_INPOS(200)(500)
GLOBAL REAL SETTLE_DC_DATA(3)(500)

AXIS_SETTLING_TIME_TEST:
    LOCAL INT axis_no
    LOCAL INT i
    LOCAL INT j
    LOCAL INT move_index
    LOCAL REAL step_distance
    LOCAL REAL target_pos
    LOCAL REAL dc_timeout_ms
    LOCAL REAL next_pos

    SETTLE_DONE = 0
    SETTLE_ERROR = 0
    SETTLE_CUR_MOVE = -1
    SETTLE_MOVE_COUNT = 0

    axis_no = SETTLE_AXIS

    if SETTLE_SAMPLE_COUNT <= 0
        SETTLE_SAMPLE_COUNT = 500
    end

    if SETTLE_SAMPLE_INTERVAL_MS <= 0
        SETTLE_SAMPLE_INTERVAL_MS = 1
    end

    if SETTLE_MOVE_TIMEOUT_MS <= 0
        SETTLE_MOVE_TIMEOUT_MS = 60000
    end

    if SETTLE_ENABLE_TIMEOUT_MS <= 0
        SETTLE_ENABLE_TIMEOUT_MS = 5000
    end
    dc_timeout_ms = (SETTLE_SAMPLE_COUNT * SETTLE_SAMPLE_INTERVAL_MS) + 2000

    if SETTLE_SAMPLE_COUNT > 500
        SETTLE_ERROR = 102
        goto test_error
    end

    SETTLE_LEFT_LIMIT = SLLIMIT(axis_no)
    SETTLE_RIGHT_LIMIT = SRLIMIT(axis_no)

    if SETTLE_RIGHT_LIMIT <= SETTLE_LEFT_LIMIT
        SETTLE_ERROR = 103
        goto test_error
    end

    if SETTLE_STEP_DISTANCE > 0
        step_distance = SETTLE_STEP_DISTANCE
    else
        if SETTLE_POINT_COUNT <= 1
            SETTLE_POINT_COUNT = 10
        end
        step_distance = (SETTLE_RIGHT_LIMIT - SETTLE_LEFT_LIMIT) / (SETTLE_POINT_COUNT - 1)
    end

    if step_distance <= 0
        SETTLE_ERROR = 101
        goto test_error
    end

    i = 0
    SETTLE_POINT_POS(i) = SETTLE_LEFT_LIMIT
    i = i + 1
    next_pos = SETTLE_LEFT_LIMIT + step_distance
    while next_pos < SETTLE_RIGHT_LIMIT
        if i >= 100
            SETTLE_ERROR = 101
            goto test_error
        end
        SETTLE_POINT_POS(i) = next_pos
        i = i + 1
        next_pos = next_pos + step_distance
    end
    if i >= 100
        SETTLE_ERROR = 101
        goto test_error
    end
    if i = 1
        SETTLE_POINT_POS(i) = SETTLE_RIGHT_LIMIT
        i = i + 1
    else
        if ABS(SETTLE_POINT_POS(i - 1) - SETTLE_RIGHT_LIMIT) > 0.000001
            SETTLE_POINT_POS(i) = SETTLE_RIGHT_LIMIT
            i = i + 1
        end
    end
    SETTLE_POINT_COUNT = i

    FCLEAR axis_no
    ENABLE axis_no
    TILL MST(axis_no).#ENABLED, SETTLE_ENABLE_TIMEOUT_MS
    if ^MST(axis_no).#ENABLED
        SETTLE_ERROR = 104
        goto test_error
    end

    PTP axis_no, SETTLE_POINT_POS(0)
    TILL ^MST(axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
    if MST(axis_no).#MOVE
        SETTLE_ERROR = 105
        goto test_error
    end

    move_index = 0

    i = 1
    LOOP (SETTLE_POINT_COUNT - 1)
        target_pos = SETTLE_POINT_POS(i)
        SETTLE_CUR_MOVE = move_index
        SETTLE_MOVE_TARGET(move_index) = target_pos
        SETTLE_MOVE_DIRECTION(move_index) = 1

        FILL(0, SETTLE_DC_DATA)
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(axis_no), AST(axis_no).#INPOS, MST(axis_no).#INPOS
        PTP axis_no, target_pos
        j = 0
        TILL ^S_ST.#DC, dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 206
            goto test_error
        end
        TILL ^MST(axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            SETTLE_ERROR = 201
            goto test_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(move_index)(j) = SETTLE_DC_DATA(0)(j)
            SETTLE_AST_INPOS(move_index)(j) = SETTLE_DC_DATA(1)(j)
            SETTLE_MST_INPOS(move_index)(j) = SETTLE_DC_DATA(2)(j)
            j = j + 1
        END

        move_index = move_index + 1
        i = i + 1
    END

    i = SETTLE_POINT_COUNT - 2
    LOOP (SETTLE_POINT_COUNT - 1)
        target_pos = SETTLE_POINT_POS(i)
        SETTLE_CUR_MOVE = move_index
        SETTLE_MOVE_TARGET(move_index) = target_pos
        SETTLE_MOVE_DIRECTION(move_index) = -1

        FILL(0, SETTLE_DC_DATA)
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(axis_no), AST(axis_no).#INPOS, MST(axis_no).#INPOS
        PTP axis_no, target_pos
        j = 0
        TILL ^S_ST.#DC, dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 207
            goto test_error
        end
        TILL ^MST(axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            SETTLE_ERROR = 202
            goto test_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(move_index)(j) = SETTLE_DC_DATA(0)(j)
            SETTLE_AST_INPOS(move_index)(j) = SETTLE_DC_DATA(1)(j)
            SETTLE_MST_INPOS(move_index)(j) = SETTLE_DC_DATA(2)(j)
            j = j + 1
        END

        move_index = move_index + 1
        i = i - 1
    END

    SETTLE_MOVE_COUNT = move_index
    SETTLE_DONE = 1
    disp "AXIS %d SETTLING TIME DONE, points=%d, moves=%d, samples=%d", axis_no, SETTLE_POINT_COUNT, SETTLE_MOVE_COUNT, SETTLE_SAMPLE_COUNT
STOP

test_error:
    STOPDC
    HALT axis_no
    disp "AXIS %d SETTLING TIME FAILED, error=%d, move=%d", axis_no, SETTLE_ERROR, SETTLE_CUR_MOVE
STOP
