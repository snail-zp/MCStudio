!
! Axis drop distance test
! Usage:
! 1. Set DROP_AXIS to the target axis number.
! 2. The test range defaults to current SLLIMIT/SRLIMIT.
! 3. Set DROP_POINT_COUNT as needed.
! 4. Run AXIS_DROP_DISTANCE_TEST.
!
! Result:
! - DROP_TARGET_POS(i)        : test positions within current soft limits
! - DROP_PRE_DISABLE_POS(i)   : position just before disable
! - DROP_POST_DROP_POS(i)     : position after drop settles
! - DROP_DISTANCE(i)          : post-drop minus pre-disable position
!

INT BUFFER_ID; BUFFER_ID = 36

GLOBAL INT DROP_AXIS
GLOBAL INT DROP_DONE
GLOBAL INT DROP_ERROR
GLOBAL INT DROP_CUR_POINT
GLOBAL INT DROP_POINT_COUNT

GLOBAL REAL DROP_LEFT_LIMIT
GLOBAL REAL DROP_RIGHT_LIMIT
GLOBAL REAL DROP_MOVE_TIMEOUT_MS
GLOBAL REAL DROP_ENABLE_TIMEOUT_MS
GLOBAL REAL DROP_DISABLE_TIMEOUT_MS
GLOBAL REAL DROP_STOP_POLL_MS
GLOBAL REAL DROP_STOP_WINDOW_MS
GLOBAL REAL DROP_STOP_TOL

GLOBAL REAL DROP_TARGET_POS(20)
GLOBAL REAL DROP_PRE_DISABLE_POS(20)
GLOBAL REAL DROP_POST_DROP_POS(20)
GLOBAL REAL DROP_DISTANCE(20)

AXIS_DROP_DISTANCE_TEST:
    LOCAL INT axis_no
    LOCAL INT i
    LOCAL REAL pitch
    LOCAL REAL prev_pos
    LOCAL REAL curr_pos
    LOCAL REAL stable_start

    DROP_DONE = 0
    DROP_ERROR = 0
    DROP_CUR_POINT = -1

    axis_no = DROP_AXIS

    if DROP_POINT_COUNT <= 0
        DROP_POINT_COUNT = 10
    end

    if DROP_MOVE_TIMEOUT_MS <= 0
        DROP_MOVE_TIMEOUT_MS = 60000
    end

    if DROP_ENABLE_TIMEOUT_MS <= 0
        DROP_ENABLE_TIMEOUT_MS = 5000
    end

    if DROP_DISABLE_TIMEOUT_MS <= 0
        DROP_DISABLE_TIMEOUT_MS = 5000
    end

    if DROP_STOP_POLL_MS <= 0
        DROP_STOP_POLL_MS = 20
    end

    if DROP_STOP_WINDOW_MS <= 0
        DROP_STOP_WINDOW_MS = 300
    end

    if DROP_STOP_TOL <= 0
        DROP_STOP_TOL = 0.001
    end

    if DROP_POINT_COUNT > 20
        DROP_ERROR = 101
        goto test_error
    end

    DROP_LEFT_LIMIT = SLLIMIT(axis_no)
    DROP_RIGHT_LIMIT = SRLIMIT(axis_no)

    if DROP_RIGHT_LIMIT <= DROP_LEFT_LIMIT
        DROP_ERROR = 102
        goto test_error
    end

    pitch = (DROP_RIGHT_LIMIT - DROP_LEFT_LIMIT) / (DROP_POINT_COUNT + 1)

    i = 0
    LOOP DROP_POINT_COUNT
        DROP_TARGET_POS(i) = DROP_LEFT_LIMIT + pitch * (i + 1)
        i = i + 1
    END

    FCLEAR axis_no
    ENABLE axis_no
    TILL MST(axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
    if ^MST(axis_no).#ENABLED
        DROP_ERROR = 103
        goto test_error
    end

    i = 0
    LOOP DROP_POINT_COUNT
        DROP_CUR_POINT = i

        PTP axis_no, DROP_TARGET_POS(i)
        TILL ^MST(axis_no).#MOVE, DROP_MOVE_TIMEOUT_MS
        if MST(axis_no).#MOVE
            DROP_ERROR = 201
            goto test_error
        end

        WAIT 100
        DROP_PRE_DISABLE_POS(i) = FPOS(axis_no)

        DISABLE axis_no
        TILL ^MST(axis_no).#ENABLED, DROP_DISABLE_TIMEOUT_MS
        if MST(axis_no).#ENABLED
            DROP_ERROR = 202
            goto test_error
        end

        prev_pos = FPOS(axis_no)
        stable_start = TIME
        while (TIME - stable_start) < DROP_STOP_WINDOW_MS
            WAIT DROP_STOP_POLL_MS
            curr_pos = FPOS(axis_no)
            if ABS(curr_pos - prev_pos) > DROP_STOP_TOL
                stable_start = TIME
            end
            prev_pos = curr_pos
        end

        DROP_POST_DROP_POS(i) = prev_pos
        DROP_DISTANCE(i) = DROP_POST_DROP_POS(i) - DROP_PRE_DISABLE_POS(i)

        FCLEAR axis_no
        ENABLE axis_no
        TILL MST(axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
        if ^MST(axis_no).#ENABLED
            DROP_ERROR = 203
            goto test_error
        end

        i = i + 1
    END

    DROP_DONE = 1
    disp "AXIS %d DROP DISTANCE DONE, points=%d", axis_no, DROP_POINT_COUNT
STOP

test_error:
    HALT axis_no
    disp "AXIS %d DROP DISTANCE FAILED, error=%d, point=%d", axis_no, DROP_ERROR, DROP_CUR_POINT
STOP
