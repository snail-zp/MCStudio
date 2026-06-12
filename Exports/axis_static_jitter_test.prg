!
! Axis static jitter test
! Usage:
! 1. Set JITTER_AXIS to the target axis number.
! 2. The test range defaults to current SLLIMIT/SRLIMIT.
! 3. Optional: override JITTER_LEFT_LIMIT / JITTER_RIGHT_LIMIT, or set point/sample parameters below.
! 3. Run AXIS_STATIC_JITTER_TEST.
!
! Result:
! - JITTER_TARGET_POS(i)        : commanded test positions within current soft limits
! - JITTER_DISABLE_STOP_POS(i)  : final position after disable and settle
! - JITTER_ENABLE_POS(i)        : position immediately after re-enable
! - JITTER_SAMPLE_DATA(i)(j)    : DC position samples at point i
!

INT BUFFER_ID; BUFFER_ID = 32

GLOBAL INT JITTER_AXIS
GLOBAL INT JITTER_DONE
GLOBAL INT JITTER_ERROR
GLOBAL INT JITTER_CUR_POINT
GLOBAL INT JITTER_POINT_COUNT
GLOBAL INT JITTER_SAMPLE_COUNT

GLOBAL REAL JITTER_LEFT_LIMIT
GLOBAL REAL JITTER_RIGHT_LIMIT
GLOBAL REAL JITTER_SAMPLE_INTERVAL_MS
GLOBAL REAL JITTER_MOVE_TIMEOUT_MS
GLOBAL REAL JITTER_ENABLE_TIMEOUT_MS
GLOBAL REAL JITTER_DISABLE_TIMEOUT_MS
GLOBAL REAL JITTER_DC_TIMEOUT_MS
GLOBAL REAL JITTER_STOP_POLL_MS
GLOBAL REAL JITTER_STOP_WINDOW_MS
GLOBAL REAL JITTER_STOP_TOL

GLOBAL REAL JITTER_TARGET_POS(20)
GLOBAL REAL JITTER_DISABLE_STOP_POS(20)
GLOBAL REAL JITTER_ENABLE_POS(20)
GLOBAL REAL JITTER_SAMPLE_DATA(20)(1000)
GLOBAL REAL JITTER_TEMP_DATA(1000)

AXIS_STATIC_JITTER_TEST:
    LOCAL INT axis_no
    LOCAL INT i
    LOCAL INT j
    LOCAL REAL pitch
    LOCAL REAL target_pos
    LOCAL REAL prev_pos
    LOCAL REAL curr_pos
    LOCAL REAL stable_start
    LOCAL REAL start_time

    JITTER_DONE = 0
    JITTER_ERROR = 0
    JITTER_CUR_POINT = -1

    axis_no = JITTER_AXIS

    if JITTER_POINT_COUNT <= 0
        JITTER_POINT_COUNT = 10
    end

    if JITTER_SAMPLE_COUNT <= 0
        JITTER_SAMPLE_COUNT = 1000
    end

    if JITTER_SAMPLE_INTERVAL_MS <= 0
        JITTER_SAMPLE_INTERVAL_MS = 1
    end

    if JITTER_MOVE_TIMEOUT_MS <= 0
        JITTER_MOVE_TIMEOUT_MS = 60000
    end

    if JITTER_ENABLE_TIMEOUT_MS <= 0
        JITTER_ENABLE_TIMEOUT_MS = 5000
    end

    if JITTER_DISABLE_TIMEOUT_MS <= 0
        JITTER_DISABLE_TIMEOUT_MS = 5000
    end

    if JITTER_STOP_POLL_MS <= 0
        JITTER_STOP_POLL_MS = 20
    end

    if JITTER_STOP_WINDOW_MS <= 0
        JITTER_STOP_WINDOW_MS = 300
    end

    if JITTER_STOP_TOL <= 0
        JITTER_STOP_TOL = 0.001
    end

    if JITTER_DC_TIMEOUT_MS <= 0
        JITTER_DC_TIMEOUT_MS = (JITTER_SAMPLE_COUNT * JITTER_SAMPLE_INTERVAL_MS) + 2000
    end

    if JITTER_POINT_COUNT > 20
        JITTER_ERROR = 101
        goto test_error
    end

    if JITTER_SAMPLE_COUNT > 1000
        JITTER_ERROR = 102
        goto test_error
    end

    JITTER_LEFT_LIMIT = SLLIMIT(axis_no)
    JITTER_RIGHT_LIMIT = SRLIMIT(axis_no)

    if JITTER_RIGHT_LIMIT <= JITTER_LEFT_LIMIT
        JITTER_ERROR = 103
        goto test_error
    end

    pitch = (JITTER_RIGHT_LIMIT - JITTER_LEFT_LIMIT) / (JITTER_POINT_COUNT + 1)

    i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_TARGET_POS(i) = JITTER_LEFT_LIMIT + pitch * (i + 1)
        i = i + 1
    END

    FCLEAR axis_no
    ENABLE axis_no
    TILL MST(axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
    if ^MST(axis_no).#ENABLED
        JITTER_ERROR = 104
        goto test_error
    end

    i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_CUR_POINT = i
        target_pos = JITTER_TARGET_POS(i)

        PTP axis_no, target_pos
        start_time = TIME
        TILL ^MST(axis_no).#MOVE, JITTER_MOVE_TIMEOUT_MS
        if (TIME - start_time) > JITTER_MOVE_TIMEOUT_MS
            JITTER_ERROR = 201
            goto test_error
        end

        WAIT 100

        DISABLE axis_no
        TILL ^MST(axis_no).#ENABLED, JITTER_DISABLE_TIMEOUT_MS
        if MST(axis_no).#ENABLED
            JITTER_ERROR = 202
            goto test_error
        end

        prev_pos = FPOS(axis_no)
        stable_start = TIME
        while (TIME - stable_start) < JITTER_STOP_WINDOW_MS
            WAIT JITTER_STOP_POLL_MS
            curr_pos = FPOS(axis_no)
            if ABS(curr_pos - prev_pos) > JITTER_STOP_TOL
                stable_start = TIME
            end
            prev_pos = curr_pos
        end
        JITTER_DISABLE_STOP_POS(i) = prev_pos

        FCLEAR axis_no
        ENABLE axis_no
        TILL MST(axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
        if ^MST(axis_no).#ENABLED
            JITTER_ERROR = 203
            goto test_error
        end
        JITTER_ENABLE_POS(i) = FPOS(axis_no)

        FILL(0, JITTER_TEMP_DATA)
        DC JITTER_TEMP_DATA, JITTER_SAMPLE_COUNT, JITTER_SAMPLE_INTERVAL_MS, FPOS(axis_no)
        TILL ^S_ST.#DC, JITTER_DC_TIMEOUT_MS
        if S_DCN < JITTER_SAMPLE_COUNT
            STOPDC
            JITTER_ERROR = 204
            goto test_error
        end

        j = 0
        LOOP JITTER_SAMPLE_COUNT
            JITTER_SAMPLE_DATA(i)(j) = JITTER_TEMP_DATA(j)
            j = j + 1
        END

        i = i + 1
    END

    JITTER_DONE = 1
    disp "AXIS %d STATIC JITTER DONE, points=%d, samples=%d", axis_no, JITTER_POINT_COUNT, JITTER_SAMPLE_COUNT
STOP

test_error:
    STOPDC
    HALT axis_no
    disp "AXIS %d STATIC JITTER FAILED, error=%d, point=%d", axis_no, JITTER_ERROR, JITTER_CUR_POINT
STOP
