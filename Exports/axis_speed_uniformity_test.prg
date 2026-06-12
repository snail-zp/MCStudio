!
! Axis speed uniformity test
! Usage:
! 1. Set UNIFORM_AXIS to the target axis number.
! 2. The travel range defaults to current SLLIMIT/SRLIMIT.
! 3. Set UNIFORM_SPEED_MIN / UNIFORM_SPEED_MAX / UNIFORM_SPEED_COUNT as needed.
! 4. Run AXIS_SPEED_UNIFORMITY_TEST.
!
! Result:
! - UNIFORM_SPEED_SET(i)              : commanded test speed
! - UNIFORM_ACC_SET(i)                : commanded acceleration = speed * 10
! - UNIFORM_JERK_SET(i)               : commanded jerk = acceleration * 10
! - UNIFORM_MOVE_DIRECTION(move)      : 1=negative to positive, -1=positive to negative
! - UNIFORM_MOVE_SPEED(move)          : commanded speed for each move
! - UNIFORM_VEL_DATA(move)(sample)    : sampled velocity data
! - UNIFORM_POS_DATA(move)(sample)    : sampled position data
!

INT BUFFER_ID; BUFFER_ID = 34

GLOBAL INT UNIFORM_AXIS
GLOBAL INT UNIFORM_DONE
GLOBAL INT UNIFORM_ERROR
GLOBAL INT UNIFORM_CUR_MOVE
GLOBAL INT UNIFORM_SPEED_COUNT
GLOBAL INT UNIFORM_SAMPLE_COUNT
GLOBAL INT UNIFORM_MOVE_COUNT

GLOBAL REAL UNIFORM_LEFT_LIMIT
GLOBAL REAL UNIFORM_RIGHT_LIMIT
GLOBAL REAL UNIFORM_SPEED_MIN
GLOBAL REAL UNIFORM_SPEED_MAX
GLOBAL REAL UNIFORM_SAMPLE_INTERVAL_MS
GLOBAL REAL UNIFORM_MOVE_TIMEOUT_MS
GLOBAL REAL UNIFORM_ENABLE_TIMEOUT_MS
GLOBAL REAL UNIFORM_SETTLE_MS

GLOBAL REAL UNIFORM_SPEED_SET(20)
GLOBAL REAL UNIFORM_ACC_SET(20)
GLOBAL REAL UNIFORM_JERK_SET(20)
GLOBAL REAL UNIFORM_MOVE_SPEED(40)
GLOBAL REAL UNIFORM_POS_DATA(40)(500)
GLOBAL REAL UNIFORM_VEL_DATA(40)(500)
GLOBAL INT UNIFORM_MOVE_DIRECTION(40)
GLOBAL REAL UNIFORM_DC_DATA(2)(500)

AXIS_SPEED_UNIFORMITY_TEST:
    LOCAL INT axis_no
    LOCAL INT i
    LOCAL INT j
    LOCAL INT move_index
    LOCAL REAL speed_step
    LOCAL REAL cmd_speed
    LOCAL REAL cmd_acc
    LOCAL REAL cmd_jerk
    LOCAL REAL start_pos
    LOCAL REAL end_pos
    LOCAL REAL dc_timeout_ms

    UNIFORM_DONE = 0
    UNIFORM_ERROR = 0
    UNIFORM_CUR_MOVE = -1
    UNIFORM_MOVE_COUNT = 0

    axis_no = UNIFORM_AXIS

    if UNIFORM_SPEED_COUNT <= 0
        UNIFORM_SPEED_COUNT = 5
    end

    if UNIFORM_SAMPLE_COUNT <= 0
        UNIFORM_SAMPLE_COUNT = 500
    end

    if UNIFORM_SAMPLE_INTERVAL_MS <= 0
        UNIFORM_SAMPLE_INTERVAL_MS = 1
    end

    if UNIFORM_MOVE_TIMEOUT_MS <= 0
        UNIFORM_MOVE_TIMEOUT_MS = 120000
    end

    if UNIFORM_ENABLE_TIMEOUT_MS <= 0
        UNIFORM_ENABLE_TIMEOUT_MS = 5000
    end

    if UNIFORM_SETTLE_MS <= 0
        UNIFORM_SETTLE_MS = 100
    end
    dc_timeout_ms = (UNIFORM_SAMPLE_COUNT * UNIFORM_SAMPLE_INTERVAL_MS) + 2000

    if UNIFORM_SPEED_COUNT > 20
        UNIFORM_ERROR = 101
        goto test_error
    end

    if UNIFORM_SAMPLE_COUNT > 500
        UNIFORM_ERROR = 102
        goto test_error
    end

    if UNIFORM_SPEED_MAX <= UNIFORM_SPEED_MIN
        UNIFORM_ERROR = 103
        goto test_error
    end

    UNIFORM_LEFT_LIMIT = SLLIMIT(axis_no)
    UNIFORM_RIGHT_LIMIT = SRLIMIT(axis_no)

    if UNIFORM_RIGHT_LIMIT <= UNIFORM_LEFT_LIMIT
        UNIFORM_ERROR = 104
        goto test_error
    end

    if UNIFORM_SPEED_COUNT = 1
        UNIFORM_SPEED_SET(0) = UNIFORM_SPEED_MIN
    else
        speed_step = (UNIFORM_SPEED_MAX - UNIFORM_SPEED_MIN) / (UNIFORM_SPEED_COUNT - 1)
        i = 0
        LOOP UNIFORM_SPEED_COUNT
            UNIFORM_SPEED_SET(i) = UNIFORM_SPEED_MIN + speed_step * i
            i = i + 1
        END
    end

    i = 0
    LOOP UNIFORM_SPEED_COUNT
        UNIFORM_ACC_SET(i) = UNIFORM_SPEED_SET(i) * 10
        UNIFORM_JERK_SET(i) = UNIFORM_ACC_SET(i) * 10
        i = i + 1
    END

    FCLEAR axis_no
    ENABLE axis_no
    TILL MST(axis_no).#ENABLED, UNIFORM_ENABLE_TIMEOUT_MS
    if ^MST(axis_no).#ENABLED
        UNIFORM_ERROR = 105
        goto test_error
    end

    move_index = 0

    i = 0
    LOOP UNIFORM_SPEED_COUNT
        cmd_speed = UNIFORM_SPEED_SET(i)
        cmd_acc = UNIFORM_ACC_SET(i)
        cmd_jerk = UNIFORM_JERK_SET(i)

        VEL(axis_no) = cmd_speed
        ACC(axis_no) = cmd_acc
        DEC(axis_no) = cmd_acc
        KDEC(axis_no) = cmd_acc
        JERK(axis_no) = cmd_jerk

        PTP axis_no, UNIFORM_LEFT_LIMIT
        TILL ^MST(axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            UNIFORM_ERROR = 201
            goto test_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = move_index
        UNIFORM_MOVE_DIRECTION(move_index) = 1
        UNIFORM_MOVE_SPEED(move_index) = cmd_speed
        start_pos = UNIFORM_LEFT_LIMIT
        end_pos = UNIFORM_RIGHT_LIMIT

        FILL(0, UNIFORM_DC_DATA)
        DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(axis_no), FVEL(axis_no)
        PTP axis_no, end_pos
        j = 0
        TILL ^S_ST.#DC, dc_timeout_ms
        if S_DCN < UNIFORM_SAMPLE_COUNT
            STOPDC
            UNIFORM_ERROR = 205
            goto test_error
        end
        TILL ^MST(axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            UNIFORM_ERROR = 202
            goto test_error
        end
        LOOP UNIFORM_SAMPLE_COUNT
            UNIFORM_POS_DATA(move_index)(j) = UNIFORM_DC_DATA(0)(j)
            UNIFORM_VEL_DATA(move_index)(j) = UNIFORM_DC_DATA(1)(j)
            j = j + 1
        END
        move_index = move_index + 1

        PTP axis_no, UNIFORM_RIGHT_LIMIT
        TILL ^MST(axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            UNIFORM_ERROR = 203
            goto test_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = move_index
        UNIFORM_MOVE_DIRECTION(move_index) = -1
        UNIFORM_MOVE_SPEED(move_index) = cmd_speed
        start_pos = UNIFORM_RIGHT_LIMIT
        end_pos = UNIFORM_LEFT_LIMIT

        FILL(0, UNIFORM_DC_DATA)
        DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(axis_no), FVEL(axis_no)
        PTP axis_no, end_pos
        j = 0
        TILL ^S_ST.#DC, dc_timeout_ms
        if S_DCN < UNIFORM_SAMPLE_COUNT
            STOPDC
            UNIFORM_ERROR = 206
            goto test_error
        end
        TILL ^MST(axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            UNIFORM_ERROR = 204
            goto test_error
        end
        LOOP UNIFORM_SAMPLE_COUNT
            UNIFORM_POS_DATA(move_index)(j) = UNIFORM_DC_DATA(0)(j)
            UNIFORM_VEL_DATA(move_index)(j) = UNIFORM_DC_DATA(1)(j)
            j = j + 1
        END
        move_index = move_index + 1

        i = i + 1
    END

    UNIFORM_MOVE_COUNT = move_index
    UNIFORM_DONE = 1
    disp "AXIS %d SPEED UNIFORMITY DONE, speed_count=%d, moves=%d, samples=%d", axis_no, UNIFORM_SPEED_COUNT, UNIFORM_MOVE_COUNT, UNIFORM_SAMPLE_COUNT
STOP

test_error:
    STOPDC
    HALT axis_no
    disp "AXIS %d SPEED UNIFORMITY FAILED, error=%d, move=%d", axis_no, UNIFORM_ERROR, UNIFORM_CUR_MOVE
STOP
