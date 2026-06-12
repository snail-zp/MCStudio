!
! MCStudio simulator axis performance buffer
! Buffer 32 is used by Axis Performance page live execution.
!

INT BUFFER_ID; BUFFER_ID = 32

GLOBAL INT LIMIT_AXIS
GLOBAL INT LIMIT_DONE
GLOBAL INT LIMIT_ERROR
GLOBAL REAL LIMIT_MARGIN_MM
GLOBAL REAL LIMIT_ELEC_NEG
GLOBAL REAL LIMIT_ELEC_POS
GLOBAL REAL LIMIT_SOFT_NEG
GLOBAL REAL LIMIT_SOFT_POS
GLOBAL REAL LIMIT_STROKE

GLOBAL INT JITTER_AXIS
GLOBAL INT JITTER_DONE
GLOBAL INT JITTER_ERROR
GLOBAL INT JITTER_POINT_COUNT
GLOBAL INT JITTER_SAMPLE_COUNT
GLOBAL REAL JITTER_LEFT_LIMIT
GLOBAL REAL JITTER_RIGHT_LIMIT
GLOBAL REAL JITTER_SAMPLE_INTERVAL_MS
GLOBAL REAL JITTER_SAMPLE_DATA(20)(1000)

GLOBAL INT SETTLE_AXIS
GLOBAL INT SETTLE_DONE
GLOBAL INT SETTLE_ERROR
GLOBAL INT SETTLE_SAMPLE_COUNT
GLOBAL INT SETTLE_MOVE_COUNT
GLOBAL REAL SETTLE_LEFT_LIMIT
GLOBAL REAL SETTLE_RIGHT_LIMIT
GLOBAL REAL SETTLE_STEP_DISTANCE
GLOBAL REAL SETTLE_SAMPLE_INTERVAL_MS
GLOBAL REAL SETTLE_POS_DATA(200)(500)
GLOBAL INT SETTLE_MST_INPOS(200)(500)

GLOBAL INT UNIFORM_AXIS
GLOBAL INT UNIFORM_DONE
GLOBAL INT UNIFORM_ERROR
GLOBAL INT UNIFORM_SPEED_COUNT
GLOBAL INT UNIFORM_SAMPLE_COUNT
GLOBAL INT UNIFORM_MOVE_COUNT
GLOBAL REAL UNIFORM_LEFT_LIMIT
GLOBAL REAL UNIFORM_RIGHT_LIMIT
GLOBAL REAL UNIFORM_SPEED_MIN
GLOBAL REAL UNIFORM_SPEED_MAX
GLOBAL REAL UNIFORM_SAMPLE_INTERVAL_MS
GLOBAL REAL UNIFORM_MOVE_SPEED(40)
GLOBAL REAL UNIFORM_VEL_DATA(40)(500)

GLOBAL INT DYNCAP_AXIS
GLOBAL INT DYNCAP_DONE
GLOBAL INT DYNCAP_ERROR
GLOBAL INT DYNCAP_SAMPLE_COUNT
GLOBAL REAL DYNCAP_LEFT_LIMIT
GLOBAL REAL DYNCAP_RIGHT_LIMIT
GLOBAL REAL DYNCAP_MAX_VEL
GLOBAL REAL DYNCAP_MAX_ACC
GLOBAL REAL DYNCAP_MAX_JERK
GLOBAL REAL DYNCAP_SAMPLE_INTERVAL_MS
GLOBAL REAL DYNCAP_VEL_DATA(2)(10000)

GLOBAL INT DROP_AXIS
GLOBAL INT DROP_DONE
GLOBAL INT DROP_ERROR
GLOBAL INT DROP_POINT_COUNT
GLOBAL REAL DROP_LEFT_LIMIT
GLOBAL REAL DROP_RIGHT_LIMIT
GLOBAL REAL DROP_DISTANCE(20)

AXIS_SOFT_LIMIT_SETUP:
    LIMIT_DONE = 0
    LIMIT_ERROR = 0

    if LIMIT_AXIS = 2
        LIMIT_ELEC_NEG = -100
        LIMIT_ELEC_POS = 100
        LIMIT_SOFT_NEG = -99
        LIMIT_SOFT_POS = 99
    else
        LIMIT_ELEC_NEG = -150
        LIMIT_ELEC_POS = 90
        LIMIT_SOFT_NEG = -149
        LIMIT_SOFT_POS = 89
    end

    LIMIT_STROKE = LIMIT_SOFT_POS - LIMIT_SOFT_NEG
    WAIT 50
    LIMIT_DONE = 1
STOP

AXIS_STATIC_JITTER_TEST:
    LOCAL INT jitter_i
    LOCAL INT jitter_j
    LOCAL REAL jitter_base

    JITTER_DONE = 0
    JITTER_ERROR = 0

    if JITTER_POINT_COUNT <= 0
        JITTER_POINT_COUNT = 8
    end
    if JITTER_SAMPLE_COUNT <= 0
        JITTER_SAMPLE_COUNT = 300
    end
    if JITTER_SAMPLE_INTERVAL_MS <= 0
        JITTER_SAMPLE_INTERVAL_MS = 1
    end

    if JITTER_AXIS = 2
        JITTER_LEFT_LIMIT = -99
        JITTER_RIGHT_LIMIT = 99
    else
        JITTER_LEFT_LIMIT = -149
        JITTER_RIGHT_LIMIT = 89
    end

    jitter_i = 0
    LOOP JITTER_POINT_COUNT
        jitter_base = JITTER_LEFT_LIMIT + ((JITTER_RIGHT_LIMIT - JITTER_LEFT_LIMIT) * (jitter_i + 1)) / (JITTER_POINT_COUNT + 1)
        jitter_j = 0
        LOOP JITTER_SAMPLE_COUNT
            JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = jitter_base
            if jitter_j > 40
                JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = jitter_base + 0.00003
            end
            if jitter_j > 120
                JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = jitter_base - 0.00002
            end
            if jitter_j > 200
                JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = jitter_base + 0.00004
            end
            if jitter_j > 260
                JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = jitter_base - 0.00001
            end
            jitter_j = jitter_j + 1
        END
        jitter_i = jitter_i + 1
    END

    WAIT 50
    JITTER_DONE = 1
STOP

AXIS_SETTLING_TIME_TEST:
    LOCAL INT settle_i
    LOCAL INT settle_j
    LOCAL REAL settle_start
    LOCAL REAL settle_target

    SETTLE_DONE = 0
    SETTLE_ERROR = 0

    if SETTLE_SAMPLE_COUNT <= 0
        SETTLE_SAMPLE_COUNT = 240
    end
    if SETTLE_SAMPLE_INTERVAL_MS <= 0
        SETTLE_SAMPLE_INTERVAL_MS = 1
    end
    if SETTLE_STEP_DISTANCE <= 0
        SETTLE_STEP_DISTANCE = 3
    end

    if SETTLE_AXIS = 2
        SETTLE_LEFT_LIMIT = -99
        SETTLE_RIGHT_LIMIT = 99
    else
        SETTLE_LEFT_LIMIT = -149
        SETTLE_RIGHT_LIMIT = 89
    end

    SETTLE_MOVE_COUNT = 6
    settle_i = 0
    LOOP SETTLE_MOVE_COUNT
        settle_start = settle_i * SETTLE_STEP_DISTANCE
        settle_target = settle_start + SETTLE_STEP_DISTANCE
        if settle_i > 2
            settle_start = settle_target
            settle_target = settle_i * SETTLE_STEP_DISTANCE
        end

        settle_j = 0
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(settle_i)(settle_j) = settle_start
            SETTLE_MST_INPOS(settle_i)(settle_j) = 0

            if settle_j > 20
                SETTLE_POS_DATA(settle_i)(settle_j) = settle_start + (settle_target - settle_start) * 0.2
            end
            if settle_j > 40
                SETTLE_POS_DATA(settle_i)(settle_j) = settle_start + (settle_target - settle_start) * 0.5
            end
            if settle_j > 60
                SETTLE_POS_DATA(settle_i)(settle_j) = settle_start + (settle_target - settle_start) * 0.85
            end
            if settle_j > 80
                SETTLE_POS_DATA(settle_i)(settle_j) = settle_target + 0.0002
            end
            if settle_j > 100
                SETTLE_POS_DATA(settle_i)(settle_j) = settle_target
                SETTLE_MST_INPOS(settle_i)(settle_j) = 1
            end

            settle_j = settle_j + 1
        END

        settle_i = settle_i + 1
    END

    WAIT 50
    SETTLE_DONE = 1
STOP

AXIS_SPEED_UNIFORMITY_TEST:
    LOCAL INT uniform_i
    LOCAL INT uniform_j
    LOCAL REAL uniform_speed_step
    LOCAL REAL uniform_cmd_speed

    UNIFORM_DONE = 0
    UNIFORM_ERROR = 0

    if UNIFORM_SPEED_COUNT <= 0
        UNIFORM_SPEED_COUNT = 4
    end
    if UNIFORM_SAMPLE_COUNT <= 0
        UNIFORM_SAMPLE_COUNT = 240
    end
    if UNIFORM_SAMPLE_INTERVAL_MS <= 0
        UNIFORM_SAMPLE_INTERVAL_MS = 1
    end
    if UNIFORM_SPEED_MIN <= 0
        UNIFORM_SPEED_MIN = 120
    end
    if UNIFORM_SPEED_MAX <= UNIFORM_SPEED_MIN
        UNIFORM_SPEED_MAX = UNIFORM_SPEED_MIN + 180
    end

    if UNIFORM_AXIS = 2
        UNIFORM_LEFT_LIMIT = -99
        UNIFORM_RIGHT_LIMIT = 99
    else
        UNIFORM_LEFT_LIMIT = -149
        UNIFORM_RIGHT_LIMIT = 89
    end

    if UNIFORM_SPEED_COUNT = 1
        uniform_speed_step = 0
    else
        uniform_speed_step = (UNIFORM_SPEED_MAX - UNIFORM_SPEED_MIN) / (UNIFORM_SPEED_COUNT - 1)
    end

    UNIFORM_MOVE_COUNT = UNIFORM_SPEED_COUNT * 2
    uniform_i = 0
    LOOP UNIFORM_SPEED_COUNT
        uniform_cmd_speed = UNIFORM_SPEED_MIN + (uniform_speed_step * uniform_i)
        UNIFORM_MOVE_SPEED(uniform_i * 2) = uniform_cmd_speed
        UNIFORM_MOVE_SPEED((uniform_i * 2) + 1) = uniform_cmd_speed

        uniform_j = 0
        LOOP UNIFORM_SAMPLE_COUNT
            UNIFORM_VEL_DATA(uniform_i * 2)(uniform_j) = uniform_cmd_speed * 0.15
            if uniform_j > 30
                UNIFORM_VEL_DATA(uniform_i * 2)(uniform_j) = uniform_cmd_speed * 0.55
            end
            if uniform_j > 60
                UNIFORM_VEL_DATA(uniform_i * 2)(uniform_j) = uniform_cmd_speed * 0.99
            end
            if uniform_j > 90
                UNIFORM_VEL_DATA(uniform_i * 2)(uniform_j) = uniform_cmd_speed * 1.005
            end
            if uniform_j > 170
                UNIFORM_VEL_DATA(uniform_i * 2)(uniform_j) = uniform_cmd_speed * 0.4
            end

            UNIFORM_VEL_DATA((uniform_i * 2) + 1)(uniform_j) = -uniform_cmd_speed * 0.15
            if uniform_j > 30
                UNIFORM_VEL_DATA((uniform_i * 2) + 1)(uniform_j) = -uniform_cmd_speed * 0.55
            end
            if uniform_j > 60
                UNIFORM_VEL_DATA((uniform_i * 2) + 1)(uniform_j) = -uniform_cmd_speed * 0.995
            end
            if uniform_j > 90
                UNIFORM_VEL_DATA((uniform_i * 2) + 1)(uniform_j) = -uniform_cmd_speed * 1.003
            end
            if uniform_j > 170
                UNIFORM_VEL_DATA((uniform_i * 2) + 1)(uniform_j) = -uniform_cmd_speed * 0.4
            end

            uniform_j = uniform_j + 1
        END

        uniform_i = uniform_i + 1
    END

    WAIT 50
    UNIFORM_DONE = 1
STOP

AXIS_DYNAMIC_CAPABILITY_TEST:
    LOCAL INT dyncap_j

    DYNCAP_DONE = 0
    DYNCAP_ERROR = 0

    if DYNCAP_SAMPLE_COUNT <= 0
        DYNCAP_SAMPLE_COUNT = 1200
    end
    if DYNCAP_SAMPLE_INTERVAL_MS <= 0
        DYNCAP_SAMPLE_INTERVAL_MS = 1
    end
    if DYNCAP_MAX_VEL <= 0
        DYNCAP_MAX_VEL = 800
    end
    if DYNCAP_MAX_ACC <= 0
        DYNCAP_MAX_ACC = 8000
    end
    if DYNCAP_MAX_JERK <= 0
        DYNCAP_MAX_JERK = 80000
    end

    if DYNCAP_AXIS = 2
        DYNCAP_LEFT_LIMIT = -99
        DYNCAP_RIGHT_LIMIT = 99
    else
        DYNCAP_LEFT_LIMIT = -149
        DYNCAP_RIGHT_LIMIT = 89
    end

    dyncap_j = 0
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(0)(dyncap_j) = 0
        if dyncap_j > 40
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL * 0.2
        end
        if dyncap_j > 80
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL * 0.45
        end
        if dyncap_j > 140
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL * 0.8
        end
        if dyncap_j > 220
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL
        end
        if dyncap_j > 700
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL * 0.6
        end
        if dyncap_j > 900
            DYNCAP_VEL_DATA(0)(dyncap_j) = DYNCAP_MAX_VEL * 0.25
        end
        if dyncap_j > 1080
            DYNCAP_VEL_DATA(0)(dyncap_j) = 0
        end
        dyncap_j = dyncap_j + 1
    END

    WAIT 50
    DYNCAP_DONE = 1
STOP

AXIS_DROP_DISTANCE_TEST:
    LOCAL INT drop_i

    DROP_DONE = 0
    DROP_ERROR = 0

    if DROP_POINT_COUNT <= 0
        DROP_POINT_COUNT = 6
    end

    DROP_LEFT_LIMIT = -60
    DROP_RIGHT_LIMIT = 60

    drop_i = 0
    LOOP DROP_POINT_COUNT
        DROP_DISTANCE(drop_i) = 0.001 + (drop_i * 0.00035)
        drop_i = drop_i + 1
    END

    WAIT 50
    DROP_DONE = 1
STOP
