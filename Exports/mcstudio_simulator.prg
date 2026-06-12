#/ Controller version = 3.13.01
#/ Date = 6/9/2026 11:36 AM
#/ User remarks = 
#23
!PNAME=0 Version
!PDESC=
!
! MCStudio simulator process buffer
! Buffer 23 is used by:
! - Common Interface commands
! - Material transfer topology
! - Workstation calibration start-position / clamp actions
!

INT BUFFER_ID; BUFFER_ID = 23

GLOBAL INT Action_Done
GLOBAL INT to_Workstation_Index
GLOBAL INT SIM_ACTIVE_STATION

GLOBAL INT DI_LoadPort_MaterialPresent
GLOBAL INT DI_Aligner_Ready
GLOBAL INT DI_ProcessA_Ready
GLOBAL INT DI_ProcessB_Ready
GLOBAL INT DI_UnloadPort_Ready
GLOBAL INT DI_Robot_Home
GLOBAL INT DI_Robot_Grip_Closed

GLOBAL INT DO_Robot_Vacuum
GLOBAL INT DO_Robot_Blower
GLOBAL INT DO_LoadPort_Clamp
GLOBAL INT DO_Aligner_Clamp
GLOBAL INT DO_ProcessA_Clamp
GLOBAL INT DO_ProcessB_Clamp
GLOBAL INT DO_UnloadPort_Clamp

GLOBAL REAL AI_Vacuum_Level
GLOBAL REAL AO_Robot_Speed_Override

GLOBAL INT x1_in_position
GLOBAL INT y1_in_position
GLOBAL INT z1_in_position
GLOBAL INT t1_in_position
GLOBAL INT x2_in_position
GLOBAL INT y2_in_position
GLOBAL INT z2_in_position
GLOBAL INT t2_in_position

Simulator_Reset:
    Action_Done = 0
    SIM_ACTIVE_STATION = 0

    DI_LoadPort_MaterialPresent = 1
    DI_Aligner_Ready = 1
    DI_ProcessA_Ready = 1
    DI_ProcessB_Ready = 1
    DI_UnloadPort_Ready = 1
    DI_Robot_Home = 1
    DI_Robot_Grip_Closed = 0

    DO_Robot_Vacuum = 0
    DO_Robot_Blower = 0
    DO_LoadPort_Clamp = 0
    DO_Aligner_Clamp = 0
    DO_ProcessA_Clamp = 0
    DO_ProcessB_Clamp = 0
    DO_UnloadPort_Clamp = 0

    AI_Vacuum_Level = 0
    AO_Robot_Speed_Override = 100

    x1_in_position = 1
    y1_in_position = 1
    z1_in_position = 1
    t1_in_position = 1
    x2_in_position = 1
    y2_in_position = 1
    z2_in_position = 1
    t2_in_position = 1

    WAIT 50
    Action_Done = 999
STOP

Fork_Clamp_Host:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    DO_Robot_Blower = 0
    AI_Vacuum_Level = 78.5
    Action_Done = 102
STOP

Fork_Unclamp_Host:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    DO_Robot_Blower = 1
    AI_Vacuum_Level = 5
    Action_Done = 101
STOP

Main_Chuck_Clamp_Host:
    Action_Done = 0
    WAIT 50
    DO_LoadPort_Clamp = 1
    Action_Done = 104
STOP

Main_Chuck_Unclamp_Host:
    Action_Done = 0
    WAIT 50
    DO_LoadPort_Clamp = 0
    Action_Done = 103
STOP

Edge_Chuck_Clamp_Host:
    Action_Done = 0
    WAIT 50
    DO_ProcessA_Clamp = 1
    Action_Done = 108
STOP

Edge_Chuck_Unclamp_Host:
    Action_Done = 0
    WAIT 50
    DO_ProcessA_Clamp = 0
    Action_Done = 107
STOP

Aligner_Clamp_Host:
    Action_Done = 0
    WAIT 50
    DO_Aligner_Clamp = 1
    Action_Done = 106
STOP

Aligner_Unclamp_Host:
    Action_Done = 0
    WAIT 50
    DO_Aligner_Clamp = 0
    Action_Done = 105
STOP

Rotary_Workstation_to_Close:
    Action_Done = 0
    WAIT 50

    x1_in_position = 0
    y1_in_position = 0
    z1_in_position = 0
    t1_in_position = 0
    x2_in_position = 0
    y2_in_position = 0
    z2_in_position = 0
    t2_in_position = 0

    if to_Workstation_Index = 0
        x1_in_position = 1
        y1_in_position = 1
        z1_in_position = 1
        t1_in_position = 1
    end

    if to_Workstation_Index = 1
        x2_in_position = 1
        y2_in_position = 1
        z2_in_position = 1
        t2_in_position = 1
    end

    if to_Workstation_Index = 2
        x1_in_position = 1
        z1_in_position = 1
        x2_in_position = 1
        z2_in_position = 1
    end

    Action_Done = 1
STOP

RobotArm_Transfer_LoadPort01:
    Action_Done = 0
    WAIT 50
    SIM_ACTIVE_STATION = 1
    DI_LoadPort_MaterialPresent = 1
    DI_Robot_Home = 0
    Action_Done = 201
STOP

RobotArm_Transfer_Aligner:
    Action_Done = 0
    WAIT 50
    SIM_ACTIVE_STATION = 2
    DI_Aligner_Ready = 1
    DI_Robot_Home = 0
    Action_Done = 202
STOP

RobotArm_Transfer_ProcessA:
    Action_Done = 0
    WAIT 50
    SIM_ACTIVE_STATION = 3
    DI_ProcessA_Ready = 1
    DI_Robot_Home = 0
    Action_Done = 203
STOP

RobotArm_Transfer_ProcessB:
    Action_Done = 0
    WAIT 50
    SIM_ACTIVE_STATION = 4
    DI_ProcessB_Ready = 1
    DI_Robot_Home = 0
    Action_Done = 204
STOP

RobotArm_Transfer_UnloadPort:
    Action_Done = 0
    WAIT 50
    SIM_ACTIVE_STATION = 5
    DI_UnloadPort_Ready = 1
    DI_Robot_Home = 1
    Action_Done = 205
STOP

RobotArm_Pick_LoadPort01:
    Action_Done = 0
    WAIT 50
    DI_LoadPort_MaterialPresent = 0
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    AI_Vacuum_Level = 82
    Action_Done = 211
STOP

RobotArm_Place_LoadPort01:
    Action_Done = 0
    WAIT 50
    DI_LoadPort_MaterialPresent = 1
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    AI_Vacuum_Level = 0
    Action_Done = 212
STOP

RobotArm_Pick_Aligner:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    AI_Vacuum_Level = 82
    Action_Done = 213
STOP

RobotArm_Place_Aligner:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    AI_Vacuum_Level = 0
    Action_Done = 214
STOP

RobotArm_Pick_ProcessA:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    AI_Vacuum_Level = 82
    Action_Done = 215
STOP

RobotArm_Place_ProcessA:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    AI_Vacuum_Level = 0
    Action_Done = 216
STOP

RobotArm_Pick_ProcessB:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    AI_Vacuum_Level = 82
    Action_Done = 217
STOP

RobotArm_Place_ProcessB:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    AI_Vacuum_Level = 0
    Action_Done = 218
STOP

RobotArm_Pick_UnloadPort:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 1
    DO_Robot_Vacuum = 1
    AI_Vacuum_Level = 82
    Action_Done = 219
STOP

RobotArm_Place_UnloadPort:
    Action_Done = 0
    WAIT 50
    DI_Robot_Grip_Closed = 0
    DO_Robot_Vacuum = 0
    AI_Vacuum_Level = 0
    Action_Done = 220
STOP

#24
!PNAME=1 X1Home
!PDESC=
REAL vel_curr
REAL acc_curr
REAL jerk_curr
REAL t_j
REAL t_a
REAL extension_line_distance
enable 0
VEL(0)= 100
ACC(0) =1000
JERK(0) =10
vel_curr = VEL(0)
acc_curr = ACC(0)
jerk_curr = JERK(0)

SET FPOS(0) = 0

REAL start_p;  start_p= -100
REAL end_p;  end_p= 100

t_j = acc_curr / jerk_curr

if vel_curr >= acc_curr * acc_curr / jerk_curr
    
    t_a = vel_curr / acc_curr - t_j
    extension_line_distance = 0.5 * acc_curr * t_a * t_a + 1.5 * acc_curr * t_a * t_j + acc_curr * t_j * t_j
else
    
    t_j = SQRT(vel_curr / jerk_curr)
    extension_line_distance = jerk_curr * t_j * t_j * t_j
end

IF(start_p < end_p )
	start_p = start_p - extension_line_distance
	end_p = end_p + extension_line_distance
ELSE
	start_p = start_p + extension_line_distance
	end_p = end_p - extension_line_distance
end


DISABLEON 24
ptp/e 0, start_p 
ENABLEON 24
ptp 0, end_p
TILL ^MST(0).#MOVE
stop

on GPHASE (0)=4
	disp FPOS(0)
ret
#31
!PNAME=2 Y1Home
!PDESC=
Fork_Clamp:
	action_done = 1
stop

Fork_Unclamp:
	action_done = 2
stop
#32
!PNAME=3 Z1Home
!PDESC=
!
! Axis performance suite buffer
! Available labels:
! - AXIS_SOFT_LIMIT_SETUP
! - AXIS_STATIC_JITTER_TEST
! - AXIS_SETTLING_TIME_TEST
! - AXIS_SPEED_UNIFORMITY_TEST
! - AXIS_DYNAMIC_CAPABILITY_TEST
! - AXIS_DROP_DISTANCE_TEST
!

INT BUFFER_ID; BUFFER_ID = 32

GLOBAL INT LIMIT_AXIS
GLOBAL INT LIMIT_DONE
GLOBAL INT LIMIT_ERROR
GLOBAL REAL LIMIT_MARGIN_MM
GLOBAL REAL LIMIT_SCAN_VEL
GLOBAL REAL LIMIT_ELEC_NEG
GLOBAL REAL LIMIT_ELEC_POS
GLOBAL REAL LIMIT_SOFT_NEG
GLOBAL REAL LIMIT_SOFT_POS
GLOBAL REAL LIMIT_STROKE

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

AXIS_SOFT_LIMIT_SETUP:
    LOCAL INT limit_axis_no

    LIMIT_DONE = 0
    LIMIT_ERROR = 0

    limit_axis_no = LIMIT_AXIS

    if LIMIT_MARGIN_MM <= 0
        LIMIT_MARGIN_MM = 1
    end

    if LIMIT_SCAN_VEL <= 0
        LIMIT_SCAN_VEL = 5
    end

    if limit_axis_no < 0
        LIMIT_ERROR = 101
        goto limit_error
    end

    disp "AXIS %d LIMIT SETUP START, margin=%.4f, scan_vel=%.4f", limit_axis_no, LIMIT_MARGIN_MM, LIMIT_SCAN_VEL

    FDEF(limit_axis_no).#RL = 0
    FDEF(limit_axis_no).#LL = 0
    FMASK(limit_axis_no).#SRL = 0
    FMASK(limit_axis_no).#SLL = 0

    FCLEAR limit_axis_no
    if ^MST(limit_axis_no).#ENABLED
        ! Log axis enable before limit scan starts.
        disp "AXIS %d LIMIT SETUP ENABLE AXIS", limit_axis_no
        ENABLE limit_axis_no
        TILL MST(limit_axis_no).#ENABLED, 5000
        if ^MST(limit_axis_no).#ENABLED
            LIMIT_ERROR = 102
            goto limit_restore
        end
    end

    disp "AXIS %d LIMIT SETUP SCAN POSITIVE", limit_axis_no
    JOG/V limit_axis_no, LIMIT_SCAN_VEL
    TILL FAULT(limit_axis_no).#RL, 120000
    if ^FAULT(limit_axis_no).#RL
        LIMIT_ERROR = 201
        goto limit_restore
    end

    JOG/V limit_axis_no, -LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(limit_axis_no).#RL, 10000
    HALT limit_axis_no
    WAIT 100

    LIMIT_ELEC_POS = FPOS(limit_axis_no)
    LIMIT_SOFT_POS = LIMIT_ELEC_POS - LIMIT_MARGIN_MM
    SRLIMIT(limit_axis_no) = LIMIT_SOFT_POS
    disp "AXIS %d LIMIT POSITIVE FOUND, elec_pos=%.4f, soft_pos=%.4f", limit_axis_no, LIMIT_ELEC_POS, LIMIT_SOFT_POS

    FCLEAR limit_axis_no
    disp "AXIS %d LIMIT SETUP SCAN NEGATIVE", limit_axis_no
    JOG/V limit_axis_no, -LIMIT_SCAN_VEL
    TILL FAULT(limit_axis_no).#LL, 120000
    if ^FAULT(limit_axis_no).#LL
        LIMIT_ERROR = 202
        goto limit_restore
    end

    JOG/V limit_axis_no, LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(limit_axis_no).#LL, 10000
    HALT limit_axis_no
    WAIT 100

    LIMIT_ELEC_NEG = FPOS(limit_axis_no)
    LIMIT_SOFT_NEG = LIMIT_ELEC_NEG + LIMIT_MARGIN_MM
    SLLIMIT(limit_axis_no) = LIMIT_SOFT_NEG
    disp "AXIS %d LIMIT NEGATIVE FOUND, elec_neg=%.4f, soft_neg=%.4f", limit_axis_no, LIMIT_ELEC_NEG, LIMIT_SOFT_NEG

    LIMIT_STROKE = LIMIT_SOFT_POS - LIMIT_SOFT_NEG

    FCLEAR limit_axis_no
    ! Log the return-to-center move after both soft limits are established.
    disp "AXIS %d LIMIT SETUP MOVE CENTER, target=%.4f", limit_axis_no, (LIMIT_SOFT_NEG + LIMIT_SOFT_POS) / 2
    ENABLE limit_axis_no
    PTP/E limit_axis_no, (LIMIT_SOFT_NEG + LIMIT_SOFT_POS) / 2

    LIMIT_DONE = 1
    disp "AXIS %d LIMITS: elec_neg=%.4f elec_pos=%.4f soft_neg=%.4f soft_pos=%.4f stroke=%.4f", limit_axis_no, LIMIT_ELEC_NEG, LIMIT_ELEC_POS, LIMIT_SOFT_NEG, LIMIT_SOFT_POS, LIMIT_STROKE

limit_restore:
    ! Log restoration of limit definitions and masks before leaving the routine.
    disp "AXIS %d LIMIT SETUP RESTORE LIMIT MASKS", limit_axis_no
    FDEF(limit_axis_no).#RL = 1
    FDEF(limit_axis_no).#LL = 1
    FMASK(limit_axis_no).#SRL = 1
    FMASK(limit_axis_no).#SLL = 1
    if LIMIT_DONE
        STOP
    end

limit_error:
    HALT limit_axis_no
    disp "AXIS %d LIMIT SETUP FAILED, error=%d", limit_axis_no, LIMIT_ERROR
    goto limit_restore

AXIS_STATIC_JITTER_TEST:
    LOCAL INT jitter_axis_no
    LOCAL INT jitter_i
    LOCAL INT jitter_j
    LOCAL REAL jitter_pitch
    LOCAL REAL jitter_target_pos
    LOCAL REAL jitter_prev_pos
    LOCAL REAL jitter_curr_pos
    LOCAL REAL jitter_stable_start
    LOCAL REAL jitter_start_time

    JITTER_DONE = 0
    JITTER_ERROR = 0
    JITTER_CUR_POINT = -1

    jitter_axis_no = JITTER_AXIS

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
        goto jitter_error
    end
    if JITTER_SAMPLE_COUNT > 1000
        JITTER_ERROR = 102
        goto jitter_error
    end
    JITTER_LEFT_LIMIT = SLLIMIT(jitter_axis_no)
    JITTER_RIGHT_LIMIT = SRLIMIT(jitter_axis_no)
    if JITTER_RIGHT_LIMIT <= JITTER_LEFT_LIMIT
        JITTER_ERROR = 103
        goto jitter_error
    end

    disp "AXIS %d STATIC JITTER START, points=%d, samples=%d, left=%.4f, right=%.4f", jitter_axis_no, JITTER_POINT_COUNT, JITTER_SAMPLE_COUNT, JITTER_LEFT_LIMIT, JITTER_RIGHT_LIMIT

    jitter_pitch = (JITTER_RIGHT_LIMIT - JITTER_LEFT_LIMIT) / (JITTER_POINT_COUNT + 1)
    jitter_i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_TARGET_POS(jitter_i) = JITTER_LEFT_LIMIT + jitter_pitch * (jitter_i + 1)
        jitter_i = jitter_i + 1
    END

    ! Log generated test points before the axis is enabled.
    disp "AXIS %d STATIC JITTER TARGETS READY, pitch=%.4f, count=%d", jitter_axis_no, jitter_pitch, JITTER_POINT_COUNT
    FCLEAR jitter_axis_no
    ! Log axis enable before jitter point traversal starts.
    disp "AXIS %d STATIC JITTER ENABLE AXIS", jitter_axis_no
    ENABLE jitter_axis_no
    TILL MST(jitter_axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
    if ^MST(jitter_axis_no).#ENABLED
        JITTER_ERROR = 104
        goto jitter_error
    end

    jitter_i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_CUR_POINT = jitter_i
        jitter_target_pos = JITTER_TARGET_POS(jitter_i)
        disp "AXIS %d STATIC JITTER POINT %d TARGET=%.4f", jitter_axis_no, jitter_i, jitter_target_pos

        PTP jitter_axis_no, jitter_target_pos
        jitter_start_time = TIME
        TILL ^MST(jitter_axis_no).#MOVE, JITTER_MOVE_TIMEOUT_MS
        if (TIME - jitter_start_time) > JITTER_MOVE_TIMEOUT_MS
            JITTER_ERROR = 201
            goto jitter_error
        end

        WAIT 100
        ! Log the disable action used to observe free-stop position drift.
        disp "AXIS %d STATIC JITTER POINT %d DISABLE AXIS", jitter_axis_no, jitter_i
        DISABLE jitter_axis_no
        TILL ^MST(jitter_axis_no).#ENABLED, JITTER_DISABLE_TIMEOUT_MS
        if MST(jitter_axis_no).#ENABLED
            JITTER_ERROR = 202
            goto jitter_error
        end

        jitter_prev_pos = FPOS(jitter_axis_no)
        jitter_stable_start = TIME
        while (TIME - jitter_stable_start) < JITTER_STOP_WINDOW_MS
            WAIT JITTER_STOP_POLL_MS
            jitter_curr_pos = FPOS(jitter_axis_no)
            if ABS(jitter_curr_pos - jitter_prev_pos) > JITTER_STOP_TOL
                jitter_stable_start = TIME
            end
            jitter_prev_pos = jitter_curr_pos
        end
        JITTER_DISABLE_STOP_POS(jitter_i) = jitter_prev_pos
        disp "AXIS %d STATIC JITTER POINT %d DISABLE_STOP=%.4f", jitter_axis_no, jitter_i, JITTER_DISABLE_STOP_POS(jitter_i)

        FCLEAR jitter_axis_no
        ENABLE jitter_axis_no
        TILL MST(jitter_axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
        if ^MST(jitter_axis_no).#ENABLED
            JITTER_ERROR = 203
            goto jitter_error
        end
        JITTER_ENABLE_POS(jitter_i) = FPOS(jitter_axis_no)
        disp "AXIS %d STATIC JITTER POINT %d ENABLE_POS=%.4f", jitter_axis_no, jitter_i, JITTER_ENABLE_POS(jitter_i)

        FILL(0, JITTER_TEMP_DATA)
        ! Log the start of DC acquisition for static position samples.
        disp "AXIS %d STATIC JITTER POINT %d DC START, samples=%d, interval_ms=%.4f", jitter_axis_no, jitter_i, JITTER_SAMPLE_COUNT, JITTER_SAMPLE_INTERVAL_MS
        DC JITTER_TEMP_DATA, JITTER_SAMPLE_COUNT, JITTER_SAMPLE_INTERVAL_MS, FPOS(jitter_axis_no)
        TILL ^S_ST.#DC, JITTER_DC_TIMEOUT_MS
        if S_DCN < JITTER_SAMPLE_COUNT
            STOPDC
            JITTER_ERROR = 204
            goto jitter_error
        end

        jitter_j = 0
        LOOP JITTER_SAMPLE_COUNT
            JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = JITTER_TEMP_DATA(jitter_j)
            jitter_j = jitter_j + 1
        END

        jitter_i = jitter_i + 1
    END

    JITTER_DONE = 1
    disp "AXIS %d STATIC JITTER DONE, points=%d, samples=%d", jitter_axis_no, JITTER_POINT_COUNT, JITTER_SAMPLE_COUNT
STOP

jitter_error:
    ! Log cleanup before stopping data collection and halting the axis.
    disp "AXIS %d STATIC JITTER CLEANUP, error=%d, point=%d", jitter_axis_no, JITTER_ERROR, JITTER_CUR_POINT
    STOPDC
    HALT jitter_axis_no
    disp "AXIS %d STATIC JITTER FAILED, error=%d, point=%d", jitter_axis_no, JITTER_ERROR, JITTER_CUR_POINT

AXIS_SETTLING_TIME_TEST:
    LOCAL INT settle_axis_no
    LOCAL INT settle_i
    LOCAL INT settle_j
    LOCAL INT settle_move_index
    LOCAL REAL settle_step_distance
    LOCAL REAL settle_target_pos
    LOCAL REAL settle_dc_timeout_ms
    LOCAL REAL settle_next_pos

    SETTLE_DONE = 0
    SETTLE_ERROR = 0
    SETTLE_CUR_MOVE = -1
    SETTLE_MOVE_COUNT = 0

    settle_axis_no = SETTLE_AXIS

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
    settle_dc_timeout_ms = (SETTLE_SAMPLE_COUNT * SETTLE_SAMPLE_INTERVAL_MS) + 2000

    if SETTLE_SAMPLE_COUNT > 500
        SETTLE_ERROR = 102
        goto settle_error
    end
    SETTLE_LEFT_LIMIT = SLLIMIT(settle_axis_no)
    SETTLE_RIGHT_LIMIT = SRLIMIT(settle_axis_no)
    if SETTLE_RIGHT_LIMIT <= SETTLE_LEFT_LIMIT
        SETTLE_ERROR = 103
        goto settle_error
    end

    if SETTLE_STEP_DISTANCE > 0
        settle_step_distance = SETTLE_STEP_DISTANCE
    else
        if SETTLE_POINT_COUNT <= 1
            SETTLE_POINT_COUNT = 10
        end
        settle_step_distance = (SETTLE_RIGHT_LIMIT - SETTLE_LEFT_LIMIT) / (SETTLE_POINT_COUNT - 1)
    end
    if settle_step_distance <= 0
        SETTLE_ERROR = 101
        goto settle_error
    end

    disp "AXIS %d SETTLING TIME START, sample_count=%d, step=%.4f, left=%.4f, right=%.4f", settle_axis_no, SETTLE_SAMPLE_COUNT, settle_step_distance, SETTLE_LEFT_LIMIT, SETTLE_RIGHT_LIMIT

    settle_i = 0
    SETTLE_POINT_POS(settle_i) = SETTLE_LEFT_LIMIT
    settle_i = settle_i + 1
    settle_next_pos = SETTLE_LEFT_LIMIT + settle_step_distance
    while settle_next_pos < SETTLE_RIGHT_LIMIT
        if settle_i >= 100
            SETTLE_ERROR = 101
            goto settle_error
        end
        SETTLE_POINT_POS(settle_i) = settle_next_pos
        settle_i = settle_i + 1
        settle_next_pos = settle_next_pos + settle_step_distance
    end
    if settle_i >= 100
        SETTLE_ERROR = 101
        goto settle_error
    end
    if settle_i = 1
        SETTLE_POINT_POS(settle_i) = SETTLE_RIGHT_LIMIT
        settle_i = settle_i + 1
    else
        if ABS(SETTLE_POINT_POS(settle_i - 1) - SETTLE_RIGHT_LIMIT) > 0.000001
            SETTLE_POINT_POS(settle_i) = SETTLE_RIGHT_LIMIT
            settle_i = settle_i + 1
        end
    end
    SETTLE_POINT_COUNT = settle_i

    ! Log the resolved settle test point layout before motion begins.
    disp "AXIS %d SETTLING TIME POINTS READY, points=%d, first=%.4f, last=%.4f", settle_axis_no, SETTLE_POINT_COUNT, SETTLE_POINT_POS(0), SETTLE_POINT_POS(SETTLE_POINT_COUNT - 1)
    FCLEAR settle_axis_no
    ! Log axis enable before settling-time motion sequence starts.
    disp "AXIS %d SETTLING TIME ENABLE AXIS", settle_axis_no
    ENABLE settle_axis_no
    TILL MST(settle_axis_no).#ENABLED, SETTLE_ENABLE_TIMEOUT_MS
    if ^MST(settle_axis_no).#ENABLED
        SETTLE_ERROR = 104
        goto settle_error
    end

    ! Log homing to the first settle point before forward and reverse scans.
    disp "AXIS %d SETTLING TIME MOVE TO START, target=%.4f", settle_axis_no, SETTLE_POINT_POS(0)
    PTP settle_axis_no, SETTLE_POINT_POS(0)
    TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
    if MST(settle_axis_no).#MOVE
        SETTLE_ERROR = 105
        goto settle_error
    end

    settle_move_index = 0
    settle_i = 1
    LOOP (SETTLE_POINT_COUNT - 1)
        settle_target_pos = SETTLE_POINT_POS(settle_i)
        SETTLE_CUR_MOVE = settle_move_index
        SETTLE_MOVE_TARGET(settle_move_index) = settle_target_pos
        SETTLE_MOVE_DIRECTION(settle_move_index) = 1
        disp "AXIS %d SETTLING TIME MOVE %d DIR=%d TARGET=%.4f", settle_axis_no, settle_move_index, SETTLE_MOVE_DIRECTION(settle_move_index), settle_target_pos
        FILL(0, SETTLE_DC_DATA)
        ! Log the start of DC capture for forward settling analysis.
        disp "AXIS %d SETTLING TIME MOVE %d DC START, samples=%d, interval_ms=%.4f", settle_axis_no, settle_move_index, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(settle_axis_no), AST(settle_axis_no).#INPOS, MST(settle_axis_no).#INPOS
        PTP settle_axis_no, settle_target_pos
        settle_j = 0
        TILL ^S_ST.#DC, settle_dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 206
            goto settle_error
        end
        TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(settle_axis_no).#MOVE
            SETTLE_ERROR = 201
            goto settle_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(settle_move_index)(settle_j) = SETTLE_DC_DATA(0)(settle_j)
            SETTLE_AST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(1)(settle_j)
            SETTLE_MST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(2)(settle_j)
            settle_j = settle_j + 1
        END
        settle_move_index = settle_move_index + 1
        settle_i = settle_i + 1
    END

    settle_i = SETTLE_POINT_COUNT - 2
    LOOP (SETTLE_POINT_COUNT - 1)
        settle_target_pos = SETTLE_POINT_POS(settle_i)
        SETTLE_CUR_MOVE = settle_move_index
        SETTLE_MOVE_TARGET(settle_move_index) = settle_target_pos
        SETTLE_MOVE_DIRECTION(settle_move_index) = -1
        disp "AXIS %d SETTLING TIME MOVE %d DIR=%d TARGET=%.4f", settle_axis_no, settle_move_index, SETTLE_MOVE_DIRECTION(settle_move_index), settle_target_pos
        FILL(0, SETTLE_DC_DATA)
        ! Log the start of DC capture for reverse settling analysis.
        disp "AXIS %d SETTLING TIME MOVE %d DC START, samples=%d, interval_ms=%.4f", settle_axis_no, settle_move_index, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(settle_axis_no), AST(settle_axis_no).#INPOS, MST(settle_axis_no).#INPOS
        PTP settle_axis_no, settle_target_pos
        settle_j = 0
        TILL ^S_ST.#DC, settle_dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 207
            goto settle_error
        end
        TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(settle_axis_no).#MOVE
            SETTLE_ERROR = 202
            goto settle_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(settle_move_index)(settle_j) = SETTLE_DC_DATA(0)(settle_j)
            SETTLE_AST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(1)(settle_j)
            SETTLE_MST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(2)(settle_j)
            settle_j = settle_j + 1
        END
        settle_move_index = settle_move_index + 1
        settle_i = settle_i - 1
    END

    SETTLE_MOVE_COUNT = settle_move_index
    SETTLE_DONE = 1
    disp "AXIS %d SETTLING TIME DONE, points=%d, moves=%d, samples=%d", settle_axis_no, SETTLE_POINT_COUNT, SETTLE_MOVE_COUNT, SETTLE_SAMPLE_COUNT
STOP

settle_error:
    ! Log cleanup before stopping data collection and halting the axis.
    disp "AXIS %d SETTLING TIME CLEANUP, error=%d, move=%d", settle_axis_no, SETTLE_ERROR, SETTLE_CUR_MOVE
    STOPDC
    HALT settle_axis_no
    disp "AXIS %d SETTLING TIME FAILED, error=%d, move=%d", settle_axis_no, SETTLE_ERROR, SETTLE_CUR_MOVE

AXIS_SPEED_UNIFORMITY_TEST:
    LOCAL INT uniform_axis_no
    LOCAL INT uniform_i
    LOCAL INT uniform_j
    LOCAL INT uniform_move_index
    LOCAL REAL uniform_speed_step
    LOCAL REAL uniform_cmd_speed
    LOCAL REAL uniform_cmd_acc
    LOCAL REAL uniform_cmd_jerk
    LOCAL REAL uniform_end_pos
    LOCAL REAL uniform_dc_timeout_ms
    LOCAL REAL uniform_dc_collect_timeout_ms
    LOCAL INT uniform_point_base_index
    LOCAL INT uniform_has_forward_phase
    LOCAL INT uniform_has_reverse_phase
    LOCAL REAL uniform_phase_wait_start

    UNIFORM_DONE = 0
    UNIFORM_ERROR = 0
    UNIFORM_CUR_MOVE = -1
    UNIFORM_MOVE_COUNT = 0
    uniform_axis_no = UNIFORM_AXIS

    if UNIFORM_SPEED_COUNT <= 0; UNIFORM_SPEED_COUNT = 5 end
    if UNIFORM_SAMPLE_COUNT <= 0; UNIFORM_SAMPLE_COUNT = 500 end
    if UNIFORM_SAMPLE_INTERVAL_MS <= 0; UNIFORM_SAMPLE_INTERVAL_MS = 1 end
    if UNIFORM_MOVE_TIMEOUT_MS <= 0; UNIFORM_MOVE_TIMEOUT_MS = 120000 end
    if UNIFORM_ENABLE_TIMEOUT_MS <= 0; UNIFORM_ENABLE_TIMEOUT_MS = 5000 end
    if UNIFORM_SETTLE_MS <= 0; UNIFORM_SETTLE_MS = 100 end
    uniform_dc_timeout_ms = (UNIFORM_SAMPLE_COUNT * UNIFORM_SAMPLE_INTERVAL_MS) + 2000
    uniform_dc_collect_timeout_ms = UNIFORM_MOVE_TIMEOUT_MS + uniform_dc_timeout_ms

    if UNIFORM_SPEED_COUNT > 20
        UNIFORM_ERROR = 101
        goto uniform_error
    end
    if UNIFORM_SAMPLE_COUNT > 500
        UNIFORM_ERROR = 102
        goto uniform_error
    end
    if UNIFORM_SPEED_MAX <= UNIFORM_SPEED_MIN
        UNIFORM_ERROR = 103
        goto uniform_error
    end
    UNIFORM_LEFT_LIMIT = SLLIMIT(uniform_axis_no)
    UNIFORM_RIGHT_LIMIT = SRLIMIT(uniform_axis_no)
    if UNIFORM_RIGHT_LIMIT <= UNIFORM_LEFT_LIMIT
        UNIFORM_ERROR = 104
        goto uniform_error
    end

    disp "AXIS %d SPEED UNIFORMITY START, speed_count=%d, samples=%d, vel_min=%.4f, vel_max=%.4f", uniform_axis_no, UNIFORM_SPEED_COUNT, UNIFORM_SAMPLE_COUNT, UNIFORM_SPEED_MIN, UNIFORM_SPEED_MAX

    if UNIFORM_SPEED_COUNT = 1
        UNIFORM_SPEED_SET(0) = UNIFORM_SPEED_MIN
    else
        uniform_speed_step = (UNIFORM_SPEED_MAX - UNIFORM_SPEED_MIN) / (UNIFORM_SPEED_COUNT - 1)
        uniform_i = 0
        LOOP UNIFORM_SPEED_COUNT
            UNIFORM_SPEED_SET(uniform_i) = UNIFORM_SPEED_MIN + uniform_speed_step * uniform_i
            uniform_i = uniform_i + 1
        END
    end

    uniform_i = 0
    LOOP UNIFORM_SPEED_COUNT
        UNIFORM_ACC_SET(uniform_i) = UNIFORM_SPEED_SET(uniform_i) * 10
        UNIFORM_JERK_SET(uniform_i) = UNIFORM_ACC_SET(uniform_i) * 10
        uniform_i = uniform_i + 1
    END

    ! Log the prepared speed sweep before the axis is enabled.
    disp "AXIS %d SPEED UNIFORMITY PROFILE READY, count=%d, first_vel=%.4f, last_vel=%.4f", uniform_axis_no, UNIFORM_SPEED_COUNT, UNIFORM_SPEED_SET(0), UNIFORM_SPEED_SET(UNIFORM_SPEED_COUNT - 1)
    FCLEAR uniform_axis_no
    ! Log axis enable before the uniformity sweep starts.
    disp "AXIS %d SPEED UNIFORMITY ENABLE AXIS", uniform_axis_no
    ENABLE uniform_axis_no
    TILL MST(uniform_axis_no).#ENABLED, UNIFORM_ENABLE_TIMEOUT_MS
    if ^MST(uniform_axis_no).#ENABLED
        UNIFORM_ERROR = 105
        goto uniform_error
    end

    uniform_move_index = 0
    uniform_i = 0
    LOOP UNIFORM_SPEED_COUNT
        uniform_point_base_index = uniform_move_index
        uniform_has_forward_phase = 0
        uniform_has_reverse_phase = 0
        uniform_cmd_speed = UNIFORM_SPEED_SET(uniform_i)
        uniform_cmd_acc = UNIFORM_ACC_SET(uniform_i)
        uniform_cmd_jerk = UNIFORM_JERK_SET(uniform_i)
        disp "AXIS %d SPEED UNIFORMITY STEP %d, vel=%.4f, acc=%.4f, jerk=%.4f", uniform_axis_no, uniform_i, uniform_cmd_speed, uniform_cmd_acc, uniform_cmd_jerk
        VEL(uniform_axis_no) = uniform_cmd_speed
        ACC(uniform_axis_no) = uniform_cmd_acc
        DEC(uniform_axis_no) = uniform_cmd_acc
        KDEC(uniform_axis_no) = uniform_cmd_acc
        JERK(uniform_axis_no) = uniform_cmd_jerk

        ! Log the reposition to the left boundary before the forward run.
        disp "AXIS %d SPEED UNIFORMITY STEP %d MOVE TO LEFT, target=%.4f", uniform_axis_no, uniform_i, UNIFORM_LEFT_LIMIT
        PTP uniform_axis_no, UNIFORM_LEFT_LIMIT
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 201
            goto uniform_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = uniform_point_base_index
        UNIFORM_MOVE_DIRECTION(uniform_point_base_index) = 1
        UNIFORM_MOVE_SPEED(uniform_point_base_index) = uniform_cmd_speed
        uniform_end_pos = UNIFORM_RIGHT_LIMIT
        disp "AXIS %d SPEED UNIFORMITY MOVE %d DIR=%d TARGET=%.4f", uniform_axis_no, uniform_point_base_index, UNIFORM_MOVE_DIRECTION(uniform_point_base_index), uniform_end_pos
        PTP uniform_axis_no, uniform_end_pos
        uniform_phase_wait_start = TIME
        while MST(uniform_axis_no).#MOVE & ^uniform_has_forward_phase
            if GPHASE(uniform_axis_no) = 4
                uniform_has_forward_phase = 1
            end
            if (TIME - uniform_phase_wait_start) > UNIFORM_MOVE_TIMEOUT_MS
                UNIFORM_ERROR = 206
                goto uniform_error
            end
            WAIT 1
        end
        if GPHASE(uniform_axis_no) = 4
            uniform_has_forward_phase = 1
            FILL(0, UNIFORM_DC_DATA)
            ! Log the start of DC capture during the forward constant-speed phase.
            disp "AXIS %d SPEED UNIFORMITY MOVE %d DC START, samples=%d, interval_ms=%.4f", uniform_axis_no, uniform_point_base_index, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS
            DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(uniform_axis_no), FVEL(uniform_axis_no)
            uniform_j = 0
            TILL ^S_ST.#DC, uniform_dc_collect_timeout_ms
            if S_DCN < UNIFORM_SAMPLE_COUNT
                STOPDC
                UNIFORM_ERROR = 206
                goto uniform_error
            end
        end
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 202
            goto uniform_error
        end
        if uniform_has_forward_phase
            LOOP UNIFORM_SAMPLE_COUNT
                UNIFORM_POS_DATA(uniform_point_base_index)(uniform_j) = UNIFORM_DC_DATA(0)(uniform_j)
                UNIFORM_VEL_DATA(uniform_point_base_index)(uniform_j) = UNIFORM_DC_DATA(1)(uniform_j)
                uniform_j = uniform_j + 1
            END
        end

        ! Log the reposition to the right boundary before the reverse run.
        disp "AXIS %d SPEED UNIFORMITY STEP %d MOVE TO RIGHT, target=%.4f", uniform_axis_no, uniform_i, UNIFORM_RIGHT_LIMIT
        PTP uniform_axis_no, UNIFORM_RIGHT_LIMIT
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 203
            goto uniform_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = uniform_point_base_index + 1
        UNIFORM_MOVE_DIRECTION(uniform_point_base_index + 1) = -1
        UNIFORM_MOVE_SPEED(uniform_point_base_index + 1) = uniform_cmd_speed
        uniform_end_pos = UNIFORM_LEFT_LIMIT
        disp "AXIS %d SPEED UNIFORMITY MOVE %d DIR=%d TARGET=%.4f", uniform_axis_no, uniform_point_base_index + 1, UNIFORM_MOVE_DIRECTION(uniform_point_base_index + 1), uniform_end_pos
        PTP uniform_axis_no, uniform_end_pos
        uniform_phase_wait_start = TIME
        while MST(uniform_axis_no).#MOVE & ^uniform_has_reverse_phase
            if GPHASE(uniform_axis_no) = 4
                uniform_has_reverse_phase = 1
            end
            if (TIME - uniform_phase_wait_start) > UNIFORM_MOVE_TIMEOUT_MS
                UNIFORM_ERROR = 208
                goto uniform_error
            end
            WAIT 1
        end
        if GPHASE(uniform_axis_no) = 4
            uniform_has_reverse_phase = 1
            FILL(0, UNIFORM_DC_DATA)
            ! Log the start of DC capture during the reverse constant-speed phase.
            disp "AXIS %d SPEED UNIFORMITY MOVE %d DC START, samples=%d, interval_ms=%.4f", uniform_axis_no, uniform_point_base_index + 1, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS
            DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(uniform_axis_no), FVEL(uniform_axis_no)
            uniform_j = 0
            TILL ^S_ST.#DC, uniform_dc_collect_timeout_ms
            if S_DCN < UNIFORM_SAMPLE_COUNT
                STOPDC
                UNIFORM_ERROR = 208
                goto uniform_error
            end
        end
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 204
            goto uniform_error
        end
        if uniform_has_reverse_phase
            LOOP UNIFORM_SAMPLE_COUNT
                UNIFORM_POS_DATA(uniform_point_base_index + 1)(uniform_j) = UNIFORM_DC_DATA(0)(uniform_j)
                UNIFORM_VEL_DATA(uniform_point_base_index + 1)(uniform_j) = UNIFORM_DC_DATA(1)(uniform_j)
                uniform_j = uniform_j + 1
            END
        end
        if uniform_has_forward_phase
            if uniform_has_reverse_phase
                uniform_move_index = uniform_move_index + 2
            end
        end
        uniform_i = uniform_i + 1
    END

    UNIFORM_MOVE_COUNT = uniform_move_index
    UNIFORM_DONE = 1
    disp "AXIS %d SPEED UNIFORMITY DONE, speed_count=%d, moves=%d, samples=%d", uniform_axis_no, UNIFORM_SPEED_COUNT, UNIFORM_MOVE_COUNT, UNIFORM_SAMPLE_COUNT
STOP

uniform_error:
    ! Log cleanup before stopping data collection and halting the axis.
    disp "AXIS %d SPEED UNIFORMITY CLEANUP, error=%d, move=%d", uniform_axis_no, UNIFORM_ERROR, UNIFORM_CUR_MOVE
    STOPDC
    HALT uniform_axis_no
    disp "AXIS %d SPEED UNIFORMITY FAILED, error=%d, move=%d", uniform_axis_no, UNIFORM_ERROR, UNIFORM_CUR_MOVE

AXIS_DYNAMIC_CAPABILITY_TEST:
    LOCAL INT dyncap_axis_no
    LOCAL INT dyncap_move_index
    LOCAL INT dyncap_j
    LOCAL REAL dyncap_start_pos
    LOCAL REAL dyncap_end_pos
    LOCAL REAL dyncap_dc_timeout_ms

    DYNCAP_DONE = 0
    DYNCAP_ERROR = 0
    DYNCAP_CUR_MOVE = -1
    DYNCAP_MOVE_COUNT = 0
    dyncap_axis_no = DYNCAP_AXIS

    if DYNCAP_SAMPLE_COUNT <= 0; DYNCAP_SAMPLE_COUNT = 5000 end
    if DYNCAP_SAMPLE_INTERVAL_MS <= 0; DYNCAP_SAMPLE_INTERVAL_MS = 1 end
    if DYNCAP_MOVE_TIMEOUT_MS <= 0; DYNCAP_MOVE_TIMEOUT_MS = 120000 end
    if DYNCAP_ENABLE_TIMEOUT_MS <= 0; DYNCAP_ENABLE_TIMEOUT_MS = 5000 end
    if DYNCAP_EDGE_MARGIN_MM <= 0; DYNCAP_EDGE_MARGIN_MM = 1 end
    if DYNCAP_SETTLE_MS <= 0; DYNCAP_SETTLE_MS = 100 end

    if DYNCAP_SAMPLE_COUNT > 10000
        DYNCAP_ERROR = 101
        goto dyncap_error
    end
    if DYNCAP_MAX_VEL <= 0
        DYNCAP_ERROR = 102
        goto dyncap_error
    end
    if DYNCAP_MAX_ACC <= 0
        DYNCAP_ERROR = 103
        goto dyncap_error
    end
    if DYNCAP_MAX_JERK <= 0
        DYNCAP_ERROR = 104
        goto dyncap_error
    end
    DYNCAP_LEFT_LIMIT = SLLIMIT(dyncap_axis_no)
    DYNCAP_RIGHT_LIMIT = SRLIMIT(dyncap_axis_no)
    if DYNCAP_RIGHT_LIMIT <= DYNCAP_LEFT_LIMIT
        DYNCAP_ERROR = 105
        goto dyncap_error
    end

    dyncap_start_pos = DYNCAP_LEFT_LIMIT + DYNCAP_EDGE_MARGIN_MM
    dyncap_end_pos = DYNCAP_RIGHT_LIMIT - DYNCAP_EDGE_MARGIN_MM
    if dyncap_end_pos <= dyncap_start_pos
        DYNCAP_ERROR = 106
        goto dyncap_error
    end
    dyncap_dc_timeout_ms = (DYNCAP_SAMPLE_COUNT * DYNCAP_SAMPLE_INTERVAL_MS) + 2000

    disp "AXIS %d DYNAMIC CAPABILITY START, samples=%d, vel=%.4f, acc=%.4f, jerk=%.4f", dyncap_axis_no, DYNCAP_SAMPLE_COUNT, DYNCAP_MAX_VEL, DYNCAP_MAX_ACC, DYNCAP_MAX_JERK

    ! Log the resolved dynamic capability travel window after applying edge margin.
    disp "AXIS %d DYNAMIC CAPABILITY WINDOW READY, start=%.4f, end=%.4f, margin=%.4f", dyncap_axis_no, dyncap_start_pos, dyncap_end_pos, DYNCAP_EDGE_MARGIN_MM
    FCLEAR dyncap_axis_no
    ! Log axis enable before the dynamic capability runs start.
    disp "AXIS %d DYNAMIC CAPABILITY ENABLE AXIS", dyncap_axis_no
    ENABLE dyncap_axis_no
    TILL MST(dyncap_axis_no).#ENABLED, DYNCAP_ENABLE_TIMEOUT_MS
    if ^MST(dyncap_axis_no).#ENABLED
        DYNCAP_ERROR = 108
        goto dyncap_error
    end

    VEL(dyncap_axis_no) = DYNCAP_MAX_VEL
    ACC(dyncap_axis_no) = DYNCAP_MAX_ACC
    DEC(dyncap_axis_no) = DYNCAP_MAX_ACC
    KDEC(dyncap_axis_no) = DYNCAP_MAX_ACC
    JERK(dyncap_axis_no) = DYNCAP_MAX_JERK

    dyncap_move_index = 0
    ! Log the move to the forward-run start position.
    disp "AXIS %d DYNAMIC CAPABILITY MOVE TO START, target=%.4f", dyncap_axis_no, dyncap_start_pos
    PTP dyncap_axis_no, dyncap_start_pos
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 201
        goto dyncap_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = dyncap_move_index
    DYNCAP_START_POS(dyncap_move_index) = dyncap_start_pos
    DYNCAP_END_POS(dyncap_move_index) = dyncap_end_pos
    DYNCAP_CMD_VEL(dyncap_move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(dyncap_move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(dyncap_move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(dyncap_move_index) = 1
    disp "AXIS %d DYNAMIC CAPABILITY MOVE %d DIR=%d START=%.4f END=%.4f", dyncap_axis_no, dyncap_move_index, DYNCAP_MOVE_DIRECTION(dyncap_move_index), dyncap_start_pos, dyncap_end_pos
    FILL(0, DYNCAP_DC_DATA)
    ! Log the start of velocity capture for the forward capability move.
    disp "AXIS %d DYNAMIC CAPABILITY MOVE %d DC START, samples=%d, interval_ms=%.4f", dyncap_axis_no, dyncap_move_index, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(dyncap_axis_no)
    PTP dyncap_axis_no, dyncap_end_pos
    dyncap_j = 0
    TILL ^S_ST.#DC, dyncap_dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 205
        goto dyncap_error
    end
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 202
        goto dyncap_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(dyncap_move_index)(dyncap_j) = DYNCAP_DC_DATA(0)(dyncap_j)
        dyncap_j = dyncap_j + 1
    END
    dyncap_move_index = dyncap_move_index + 1

    ! Log the reposition to the reverse-run start position.
    disp "AXIS %d DYNAMIC CAPABILITY MOVE TO END, target=%.4f", dyncap_axis_no, dyncap_end_pos
    PTP dyncap_axis_no, dyncap_end_pos
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 203
        goto dyncap_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = dyncap_move_index
    DYNCAP_START_POS(dyncap_move_index) = dyncap_end_pos
    DYNCAP_END_POS(dyncap_move_index) = dyncap_start_pos
    DYNCAP_CMD_VEL(dyncap_move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(dyncap_move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(dyncap_move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(dyncap_move_index) = -1
    disp "AXIS %d DYNAMIC CAPABILITY MOVE %d DIR=%d START=%.4f END=%.4f", dyncap_axis_no, dyncap_move_index, DYNCAP_MOVE_DIRECTION(dyncap_move_index), dyncap_end_pos, dyncap_start_pos
    FILL(0, DYNCAP_DC_DATA)
    ! Log the start of velocity capture for the reverse capability move.
    disp "AXIS %d DYNAMIC CAPABILITY MOVE %d DC START, samples=%d, interval_ms=%.4f", dyncap_axis_no, dyncap_move_index, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(dyncap_axis_no)
    PTP dyncap_axis_no, dyncap_start_pos
    dyncap_j = 0
    TILL ^S_ST.#DC, dyncap_dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 206
        goto dyncap_error
    end
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 204
        goto dyncap_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(dyncap_move_index)(dyncap_j) = DYNCAP_DC_DATA(0)(dyncap_j)
        dyncap_j = dyncap_j + 1
    END
    dyncap_move_index = dyncap_move_index + 1

    DYNCAP_MOVE_COUNT = dyncap_move_index
    DYNCAP_DONE = 1
    disp "AXIS %d DYNAMIC CAPABILITY DONE, moves=%d, samples=%d", dyncap_axis_no, DYNCAP_MOVE_COUNT, DYNCAP_SAMPLE_COUNT
STOP

dyncap_error:
    ! Log cleanup before stopping data collection and halting the axis.
    disp "AXIS %d DYNAMIC CAPABILITY CLEANUP, error=%d, move=%d", dyncap_axis_no, DYNCAP_ERROR, DYNCAP_CUR_MOVE
    STOPDC
    HALT dyncap_axis_no
    disp "AXIS %d DYNAMIC CAPABILITY FAILED, error=%d, move=%d", dyncap_axis_no, DYNCAP_ERROR, DYNCAP_CUR_MOVE

AXIS_DROP_DISTANCE_TEST:
    LOCAL INT drop_axis_no
    LOCAL INT drop_i
    LOCAL REAL drop_pitch
    LOCAL REAL drop_prev_pos
    LOCAL REAL drop_curr_pos
    LOCAL REAL drop_stable_start

    DROP_DONE = 0
    DROP_ERROR = 0
    DROP_CUR_POINT = -1
    drop_axis_no = DROP_AXIS

    if DROP_POINT_COUNT <= 0; DROP_POINT_COUNT = 10 end
    if DROP_MOVE_TIMEOUT_MS <= 0; DROP_MOVE_TIMEOUT_MS = 60000 end
    if DROP_ENABLE_TIMEOUT_MS <= 0; DROP_ENABLE_TIMEOUT_MS = 5000 end
    if DROP_DISABLE_TIMEOUT_MS <= 0; DROP_DISABLE_TIMEOUT_MS = 5000 end
    if DROP_STOP_POLL_MS <= 0; DROP_STOP_POLL_MS = 20 end
    if DROP_STOP_WINDOW_MS <= 0; DROP_STOP_WINDOW_MS = 300 end
    if DROP_STOP_TOL <= 0; DROP_STOP_TOL = 0.001 end

    if DROP_POINT_COUNT > 20
        DROP_ERROR = 101
        goto drop_error
    end
    DROP_LEFT_LIMIT = SLLIMIT(drop_axis_no)
    DROP_RIGHT_LIMIT = SRLIMIT(drop_axis_no)
    if DROP_RIGHT_LIMIT <= DROP_LEFT_LIMIT
        DROP_ERROR = 102
        goto drop_error
    end

    disp "AXIS %d DROP DISTANCE START, points=%d, left=%.4f, right=%.4f", drop_axis_no, DROP_POINT_COUNT, DROP_LEFT_LIMIT, DROP_RIGHT_LIMIT

    drop_pitch = (DROP_RIGHT_LIMIT - DROP_LEFT_LIMIT) / (DROP_POINT_COUNT + 1)
    drop_i = 0
    LOOP DROP_POINT_COUNT
        DROP_TARGET_POS(drop_i) = DROP_LEFT_LIMIT + drop_pitch * (drop_i + 1)
        drop_i = drop_i + 1
    END

    ! Log generated drop test points before the axis is enabled.
    disp "AXIS %d DROP DISTANCE TARGETS READY, pitch=%.4f, count=%d", drop_axis_no, drop_pitch, DROP_POINT_COUNT
    FCLEAR drop_axis_no
    ! Log axis enable before drop-distance traversal starts.
    disp "AXIS %d DROP DISTANCE ENABLE AXIS", drop_axis_no
    ENABLE drop_axis_no
    TILL MST(drop_axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
    if ^MST(drop_axis_no).#ENABLED
        DROP_ERROR = 103
        goto drop_error
    end

    drop_i = 0
    LOOP DROP_POINT_COUNT
        DROP_CUR_POINT = drop_i
        disp "AXIS %d DROP DISTANCE POINT %d TARGET=%.4f", drop_axis_no, drop_i, DROP_TARGET_POS(drop_i)
        PTP drop_axis_no, DROP_TARGET_POS(drop_i)
        TILL ^MST(drop_axis_no).#MOVE, DROP_MOVE_TIMEOUT_MS
        if MST(drop_axis_no).#MOVE
            DROP_ERROR = 201
            goto drop_error
        end

        WAIT 100
        DROP_PRE_DISABLE_POS(drop_i) = FPOS(drop_axis_no)
        ! Log the disable action used to observe gravity or passive drop distance.
        disp "AXIS %d DROP DISTANCE POINT %d DISABLE AXIS, pre=%.4f", drop_axis_no, drop_i, DROP_PRE_DISABLE_POS(drop_i)
        DISABLE drop_axis_no
        TILL ^MST(drop_axis_no).#ENABLED, DROP_DISABLE_TIMEOUT_MS
        if MST(drop_axis_no).#ENABLED
            DROP_ERROR = 202
            goto drop_error
        end

        drop_prev_pos = FPOS(drop_axis_no)
        drop_stable_start = TIME
        while (TIME - drop_stable_start) < DROP_STOP_WINDOW_MS
            WAIT DROP_STOP_POLL_MS
            drop_curr_pos = FPOS(drop_axis_no)
            if ABS(drop_curr_pos - drop_prev_pos) > DROP_STOP_TOL
                drop_stable_start = TIME
            end
            drop_prev_pos = drop_curr_pos
        end

        DROP_POST_DROP_POS(drop_i) = drop_prev_pos
        DROP_DISTANCE(drop_i) = DROP_POST_DROP_POS(drop_i) - DROP_PRE_DISABLE_POS(drop_i)
        disp "AXIS %d DROP DISTANCE POINT %d PRE=%.4f POST=%.4f DROP=%.4f", drop_axis_no, drop_i, DROP_PRE_DISABLE_POS(drop_i), DROP_POST_DROP_POS(drop_i), DROP_DISTANCE(drop_i)
        FCLEAR drop_axis_no
        ! Log axis re-enable before continuing to the next drop point.
        disp "AXIS %d DROP DISTANCE POINT %d RE-ENABLE AXIS", drop_axis_no, drop_i
        ENABLE drop_axis_no
        TILL MST(drop_axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
        if ^MST(drop_axis_no).#ENABLED
            DROP_ERROR = 203
            goto drop_error
        end
        drop_i = drop_i + 1
    END

    DROP_DONE = 1
    disp "AXIS %d DROP DISTANCE DONE, points=%d", drop_axis_no, DROP_POINT_COUNT
STOP

drop_error:
    ! Log cleanup before halting the axis after a drop-distance failure.
    disp "AXIS %d DROP DISTANCE CLEANUP, error=%d, point=%d", drop_axis_no, DROP_ERROR, DROP_CUR_POINT
    HALT drop_axis_no
    disp "AXIS %d DROP DISTANCE FAILED, error=%d, point=%d", drop_axis_no, DROP_ERROR, DROP_CUR_POINT

#40
!PNAME=4 T1Home
!PDESC=
!
! Axis performance suite buffer
! Available labels:
! - AXIS_SOFT_LIMIT_SETUP
! - AXIS_STATIC_JITTER_TEST
! - AXIS_SETTLING_TIME_TEST
! - AXIS_SPEED_UNIFORMITY_TEST
! - AXIS_DYNAMIC_CAPABILITY_TEST
! - AXIS_DROP_DISTANCE_TEST
!

INT BUFFER_ID; BUFFER_ID = 40

GLOBAL INT LIMIT_AXIS
GLOBAL INT LIMIT_DONE
GLOBAL INT LIMIT_ERROR
GLOBAL REAL LIMIT_MARGIN_MM
GLOBAL REAL LIMIT_SCAN_VEL
GLOBAL REAL LIMIT_ELEC_NEG
GLOBAL REAL LIMIT_ELEC_POS
GLOBAL REAL LIMIT_SOFT_NEG
GLOBAL REAL LIMIT_SOFT_POS
GLOBAL REAL LIMIT_STROKE

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

AXIS_SOFT_LIMIT_SETUP:
    LOCAL INT limit_axis_no

    LIMIT_DONE = 0
    LIMIT_ERROR = 0

    limit_axis_no = LIMIT_AXIS

    if LIMIT_MARGIN_MM <= 0
        LIMIT_MARGIN_MM = 1
    end

    if LIMIT_SCAN_VEL <= 0
        LIMIT_SCAN_VEL = 5
    end

    if limit_axis_no < 0
        LIMIT_ERROR = 101
        goto limit_error
    end

    FDEF(limit_axis_no).#RL = 0
    FDEF(limit_axis_no).#LL = 0
    FMASK(limit_axis_no).#SRL = 0
    FMASK(limit_axis_no).#SLL = 0

    FCLEAR limit_axis_no
    if ^MST(limit_axis_no).#ENABLED
        ENABLE limit_axis_no
        TILL MST(limit_axis_no).#ENABLED, 5000
        if ^MST(limit_axis_no).#ENABLED
            LIMIT_ERROR = 102
            goto limit_restore
        end
    end

    JOG/V limit_axis_no, LIMIT_SCAN_VEL
    TILL FAULT(limit_axis_no).#RL, 120000
    if ^FAULT(limit_axis_no).#RL
        LIMIT_ERROR = 201
        goto limit_restore
    end

    JOG/V limit_axis_no, -LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(limit_axis_no).#RL, 10000
    HALT limit_axis_no
    WAIT 100

    LIMIT_ELEC_POS = FPOS(limit_axis_no)
    LIMIT_SOFT_POS = LIMIT_ELEC_POS - LIMIT_MARGIN_MM
    SRLIMIT(limit_axis_no) = LIMIT_SOFT_POS

    FCLEAR limit_axis_no
    JOG/V limit_axis_no, -LIMIT_SCAN_VEL
    TILL FAULT(limit_axis_no).#LL, 120000
    if ^FAULT(limit_axis_no).#LL
        LIMIT_ERROR = 202
        goto limit_restore
    end

    JOG/V limit_axis_no, LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(limit_axis_no).#LL, 10000
    HALT limit_axis_no
    WAIT 100

    LIMIT_ELEC_NEG = FPOS(limit_axis_no)
    LIMIT_SOFT_NEG = LIMIT_ELEC_NEG + LIMIT_MARGIN_MM
    SLLIMIT(limit_axis_no) = LIMIT_SOFT_NEG

    LIMIT_STROKE = LIMIT_SOFT_POS - LIMIT_SOFT_NEG
    if LIMIT_STROKE <= 0
        LIMIT_ERROR = 203
        goto limit_restore
    end

    FCLEAR limit_axis_no
    ENABLE limit_axis_no
    PTP/E limit_axis_no, (LIMIT_SOFT_NEG + LIMIT_SOFT_POS) / 2

    LIMIT_DONE = 1
    disp "AXIS %d LIMITS: elec_neg=%.4f elec_pos=%.4f soft_neg=%.4f soft_pos=%.4f stroke=%.4f", limit_axis_no, LIMIT_ELEC_NEG, LIMIT_ELEC_POS, LIMIT_SOFT_NEG, LIMIT_SOFT_POS, LIMIT_STROKE

limit_restore:
    FDEF(limit_axis_no).#RL = 1
    FDEF(limit_axis_no).#LL = 1
    FMASK(limit_axis_no).#SRL = 1
    FMASK(limit_axis_no).#SLL = 1
    if LIMIT_DONE
        STOP
    end

limit_error:
    HALT limit_axis_no
    disp "AXIS %d LIMIT SETUP FAILED, error=%d", limit_axis_no, LIMIT_ERROR
    goto limit_restore

AXIS_STATIC_JITTER_TEST:
    LOCAL INT jitter_axis_no
    LOCAL INT jitter_i
    LOCAL INT jitter_j
    LOCAL REAL jitter_pitch
    LOCAL REAL jitter_target_pos
    LOCAL REAL jitter_prev_pos
    LOCAL REAL jitter_curr_pos
    LOCAL REAL jitter_stable_start
    LOCAL REAL jitter_start_time

    JITTER_DONE = 0
    JITTER_ERROR = 0
    JITTER_CUR_POINT = -1

    jitter_axis_no = JITTER_AXIS

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
        goto jitter_error
    end
    if JITTER_SAMPLE_COUNT > 1000
        JITTER_ERROR = 102
        goto jitter_error
    end
    if JITTER_RIGHT_LIMIT <= JITTER_LEFT_LIMIT
        JITTER_LEFT_LIMIT = SLLIMIT(jitter_axis_no)
        JITTER_RIGHT_LIMIT = SRLIMIT(jitter_axis_no)
    end
    if JITTER_RIGHT_LIMIT <= JITTER_LEFT_LIMIT
        JITTER_ERROR = 103
        goto jitter_error
    end

    jitter_pitch = (JITTER_RIGHT_LIMIT - JITTER_LEFT_LIMIT) / (JITTER_POINT_COUNT + 1)
    jitter_i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_TARGET_POS(jitter_i) = JITTER_LEFT_LIMIT + jitter_pitch * (jitter_i + 1)
        jitter_i = jitter_i + 1
    END

    FCLEAR jitter_axis_no
    ENABLE jitter_axis_no
    TILL MST(jitter_axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
    if ^MST(jitter_axis_no).#ENABLED
        JITTER_ERROR = 104
        goto jitter_error
    end

    jitter_i = 0
    LOOP JITTER_POINT_COUNT
        JITTER_CUR_POINT = jitter_i
        jitter_target_pos = JITTER_TARGET_POS(jitter_i)

        PTP jitter_axis_no, jitter_target_pos
        jitter_start_time = TIME
        TILL ^MST(jitter_axis_no).#MOVE, JITTER_MOVE_TIMEOUT_MS
        if (TIME - jitter_start_time) > JITTER_MOVE_TIMEOUT_MS
            JITTER_ERROR = 201
            goto jitter_error
        end

        WAIT 100
        DISABLE jitter_axis_no
        TILL ^MST(jitter_axis_no).#ENABLED, JITTER_DISABLE_TIMEOUT_MS
        if MST(jitter_axis_no).#ENABLED
            JITTER_ERROR = 202
            goto jitter_error
        end

        jitter_prev_pos = FPOS(jitter_axis_no)
        jitter_stable_start = TIME
        while (TIME - jitter_stable_start) < JITTER_STOP_WINDOW_MS
            WAIT JITTER_STOP_POLL_MS
            jitter_curr_pos = FPOS(jitter_axis_no)
            if ABS(jitter_curr_pos - jitter_prev_pos) > JITTER_STOP_TOL
                jitter_stable_start = TIME
            end
            jitter_prev_pos = jitter_curr_pos
        end
        JITTER_DISABLE_STOP_POS(jitter_i) = jitter_prev_pos

        FCLEAR jitter_axis_no
        ENABLE jitter_axis_no
        TILL MST(jitter_axis_no).#ENABLED, JITTER_ENABLE_TIMEOUT_MS
        if ^MST(jitter_axis_no).#ENABLED
            JITTER_ERROR = 203
            goto jitter_error
        end
        JITTER_ENABLE_POS(jitter_i) = FPOS(jitter_axis_no)

        FILL(0, JITTER_TEMP_DATA)
        DC JITTER_TEMP_DATA, JITTER_SAMPLE_COUNT, JITTER_SAMPLE_INTERVAL_MS, FPOS(jitter_axis_no)
        TILL ^S_ST.#DC, JITTER_DC_TIMEOUT_MS
        if S_DCN < JITTER_SAMPLE_COUNT
            STOPDC
            JITTER_ERROR = 204
            goto jitter_error
        end

        jitter_j = 0
        LOOP JITTER_SAMPLE_COUNT
            JITTER_SAMPLE_DATA(jitter_i)(jitter_j) = JITTER_TEMP_DATA(jitter_j)
            jitter_j = jitter_j + 1
        END

        jitter_i = jitter_i + 1
    END

    JITTER_DONE = 1
    disp "AXIS %d STATIC JITTER DONE, points=%d, samples=%d", jitter_axis_no, JITTER_POINT_COUNT, JITTER_SAMPLE_COUNT
STOP

jitter_error:
    STOPDC
    HALT jitter_axis_no
    disp "AXIS %d STATIC JITTER FAILED, error=%d, point=%d", jitter_axis_no, JITTER_ERROR, JITTER_CUR_POINT

AXIS_SETTLING_TIME_TEST:
    LOCAL INT settle_axis_no
    LOCAL INT settle_i
    LOCAL INT settle_j
    LOCAL INT settle_move_index
    LOCAL REAL settle_step_distance
    LOCAL REAL settle_target_pos
    LOCAL REAL settle_dc_timeout_ms
    LOCAL REAL settle_next_pos

    SETTLE_DONE = 0
    SETTLE_ERROR = 0
    SETTLE_CUR_MOVE = -1
    SETTLE_MOVE_COUNT = 0

    settle_axis_no = SETTLE_AXIS

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
    settle_dc_timeout_ms = (SETTLE_SAMPLE_COUNT * SETTLE_SAMPLE_INTERVAL_MS) + 2000

    if SETTLE_SAMPLE_COUNT > 500
        SETTLE_ERROR = 102
        goto settle_error
    end
    if SETTLE_RIGHT_LIMIT <= SETTLE_LEFT_LIMIT
        SETTLE_LEFT_LIMIT = SLLIMIT(settle_axis_no)
        SETTLE_RIGHT_LIMIT = SRLIMIT(settle_axis_no)
    end
    if SETTLE_RIGHT_LIMIT <= SETTLE_LEFT_LIMIT
        SETTLE_ERROR = 103
        goto settle_error
    end

    if SETTLE_STEP_DISTANCE > 0
        settle_step_distance = SETTLE_STEP_DISTANCE
    else
        if SETTLE_POINT_COUNT <= 1
            SETTLE_POINT_COUNT = 10
        end
        settle_step_distance = (SETTLE_RIGHT_LIMIT - SETTLE_LEFT_LIMIT) / (SETTLE_POINT_COUNT - 1)
    end
    if settle_step_distance <= 0
        SETTLE_ERROR = 101
        goto settle_error
    end

    settle_i = 0
    SETTLE_POINT_POS(settle_i) = SETTLE_LEFT_LIMIT
    settle_i = settle_i + 1
    settle_next_pos = SETTLE_LEFT_LIMIT + settle_step_distance
    while settle_next_pos < SETTLE_RIGHT_LIMIT
        if settle_i >= 100
            SETTLE_ERROR = 101
            goto settle_error
        end
        SETTLE_POINT_POS(settle_i) = settle_next_pos
        settle_i = settle_i + 1
        settle_next_pos = settle_next_pos + settle_step_distance
    end
    if settle_i >= 100
        SETTLE_ERROR = 101
        goto settle_error
    end
    if settle_i = 1
        SETTLE_POINT_POS(settle_i) = SETTLE_RIGHT_LIMIT
        settle_i = settle_i + 1
    else
        if ABS(SETTLE_POINT_POS(settle_i - 1) - SETTLE_RIGHT_LIMIT) > 0.000001
            SETTLE_POINT_POS(settle_i) = SETTLE_RIGHT_LIMIT
            settle_i = settle_i + 1
        end
    end
    SETTLE_POINT_COUNT = settle_i

    FCLEAR settle_axis_no
    ENABLE settle_axis_no
    TILL MST(settle_axis_no).#ENABLED, SETTLE_ENABLE_TIMEOUT_MS
    if ^MST(settle_axis_no).#ENABLED
        SETTLE_ERROR = 104
        goto settle_error
    end

    PTP settle_axis_no, SETTLE_POINT_POS(0)
    TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
    if MST(settle_axis_no).#MOVE
        SETTLE_ERROR = 105
        goto settle_error
    end

    settle_move_index = 0
    settle_i = 1
    LOOP (SETTLE_POINT_COUNT - 1)
        settle_target_pos = SETTLE_POINT_POS(settle_i)
        SETTLE_CUR_MOVE = settle_move_index
        SETTLE_MOVE_TARGET(settle_move_index) = settle_target_pos
        SETTLE_MOVE_DIRECTION(settle_move_index) = 1
        FILL(0, SETTLE_DC_DATA)
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(settle_axis_no), AST(settle_axis_no).#INPOS, MST(settle_axis_no).#INPOS
        PTP settle_axis_no, settle_target_pos
        settle_j = 0
        TILL ^S_ST.#DC, settle_dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 206
            goto settle_error
        end
        TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(settle_axis_no).#MOVE
            SETTLE_ERROR = 201
            goto settle_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(settle_move_index)(settle_j) = SETTLE_DC_DATA(0)(settle_j)
            SETTLE_AST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(1)(settle_j)
            SETTLE_MST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(2)(settle_j)
            settle_j = settle_j + 1
        END
        settle_move_index = settle_move_index + 1
        settle_i = settle_i + 1
    END

    settle_i = SETTLE_POINT_COUNT - 2
    LOOP (SETTLE_POINT_COUNT - 1)
        settle_target_pos = SETTLE_POINT_POS(settle_i)
        SETTLE_CUR_MOVE = settle_move_index
        SETTLE_MOVE_TARGET(settle_move_index) = settle_target_pos
        SETTLE_MOVE_DIRECTION(settle_move_index) = -1
        FILL(0, SETTLE_DC_DATA)
        DC SETTLE_DC_DATA, SETTLE_SAMPLE_COUNT, SETTLE_SAMPLE_INTERVAL_MS, FPOS(settle_axis_no), AST(settle_axis_no).#INPOS, MST(settle_axis_no).#INPOS
        PTP settle_axis_no, settle_target_pos
        settle_j = 0
        TILL ^S_ST.#DC, settle_dc_timeout_ms
        if S_DCN < SETTLE_SAMPLE_COUNT
            STOPDC
            SETTLE_ERROR = 207
            goto settle_error
        end
        TILL ^MST(settle_axis_no).#MOVE, SETTLE_MOVE_TIMEOUT_MS
        if MST(settle_axis_no).#MOVE
            SETTLE_ERROR = 202
            goto settle_error
        end
        LOOP SETTLE_SAMPLE_COUNT
            SETTLE_POS_DATA(settle_move_index)(settle_j) = SETTLE_DC_DATA(0)(settle_j)
            SETTLE_AST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(1)(settle_j)
            SETTLE_MST_INPOS(settle_move_index)(settle_j) = SETTLE_DC_DATA(2)(settle_j)
            settle_j = settle_j + 1
        END
        settle_move_index = settle_move_index + 1
        settle_i = settle_i - 1
    END

    SETTLE_MOVE_COUNT = settle_move_index
    SETTLE_DONE = 1
    disp "AXIS %d SETTLING TIME DONE, points=%d, moves=%d, samples=%d", settle_axis_no, SETTLE_POINT_COUNT, SETTLE_MOVE_COUNT, SETTLE_SAMPLE_COUNT
STOP

settle_error:
    STOPDC
    HALT settle_axis_no
    disp "AXIS %d SETTLING TIME FAILED, error=%d, move=%d", settle_axis_no, SETTLE_ERROR, SETTLE_CUR_MOVE

AXIS_SPEED_UNIFORMITY_TEST:
    LOCAL INT uniform_axis_no
    LOCAL INT uniform_i
    LOCAL INT uniform_j
    LOCAL INT uniform_move_index
    LOCAL REAL uniform_speed_step
    LOCAL REAL uniform_cmd_speed
    LOCAL REAL uniform_cmd_acc
    LOCAL REAL uniform_cmd_jerk
    LOCAL REAL uniform_end_pos
    LOCAL REAL uniform_dc_timeout_ms
    LOCAL REAL uniform_dc_collect_timeout_ms
    LOCAL INT uniform_point_base_index
    LOCAL INT uniform_has_forward_phase
    LOCAL INT uniform_has_reverse_phase

    UNIFORM_DONE = 0
    UNIFORM_ERROR = 0
    UNIFORM_CUR_MOVE = -1
    UNIFORM_MOVE_COUNT = 0
    uniform_axis_no = UNIFORM_AXIS

    if UNIFORM_SPEED_COUNT <= 0; UNIFORM_SPEED_COUNT = 5 end
    if UNIFORM_SAMPLE_COUNT <= 0; UNIFORM_SAMPLE_COUNT = 500 end
    if UNIFORM_SAMPLE_INTERVAL_MS <= 0; UNIFORM_SAMPLE_INTERVAL_MS = 1 end
    if UNIFORM_MOVE_TIMEOUT_MS <= 0; UNIFORM_MOVE_TIMEOUT_MS = 120000 end
    if UNIFORM_ENABLE_TIMEOUT_MS <= 0; UNIFORM_ENABLE_TIMEOUT_MS = 5000 end
    if UNIFORM_SETTLE_MS <= 0; UNIFORM_SETTLE_MS = 100 end
    uniform_dc_timeout_ms = (UNIFORM_SAMPLE_COUNT * UNIFORM_SAMPLE_INTERVAL_MS) + 2000
    uniform_dc_collect_timeout_ms = UNIFORM_MOVE_TIMEOUT_MS + uniform_dc_timeout_ms

    if UNIFORM_SPEED_COUNT > 20
        UNIFORM_ERROR = 101
        goto uniform_error
    end
    if UNIFORM_SAMPLE_COUNT > 500
        UNIFORM_ERROR = 102
        goto uniform_error
    end
    if UNIFORM_SPEED_MAX <= UNIFORM_SPEED_MIN
        UNIFORM_ERROR = 103
        goto uniform_error
    end
    if UNIFORM_RIGHT_LIMIT <= UNIFORM_LEFT_LIMIT
        UNIFORM_LEFT_LIMIT = SLLIMIT(uniform_axis_no)
        UNIFORM_RIGHT_LIMIT = SRLIMIT(uniform_axis_no)
    end
    if UNIFORM_RIGHT_LIMIT <= UNIFORM_LEFT_LIMIT
        UNIFORM_ERROR = 104
        goto uniform_error
    end

    if UNIFORM_SPEED_COUNT = 1
        UNIFORM_SPEED_SET(0) = UNIFORM_SPEED_MIN
    else
        uniform_speed_step = (UNIFORM_SPEED_MAX - UNIFORM_SPEED_MIN) / (UNIFORM_SPEED_COUNT - 1)
        uniform_i = 0
        LOOP UNIFORM_SPEED_COUNT
            UNIFORM_SPEED_SET(uniform_i) = UNIFORM_SPEED_MIN + uniform_speed_step * uniform_i
            uniform_i = uniform_i + 1
        END
    end

    uniform_i = 0
    LOOP UNIFORM_SPEED_COUNT
        UNIFORM_ACC_SET(uniform_i) = UNIFORM_SPEED_SET(uniform_i) * 10
        UNIFORM_JERK_SET(uniform_i) = UNIFORM_ACC_SET(uniform_i) * 10
        uniform_i = uniform_i + 1
    END

    FCLEAR uniform_axis_no
    ENABLE uniform_axis_no
    TILL MST(uniform_axis_no).#ENABLED, UNIFORM_ENABLE_TIMEOUT_MS
    if ^MST(uniform_axis_no).#ENABLED
        UNIFORM_ERROR = 105
        goto uniform_error
    end

    uniform_move_index = 0
    uniform_i = 0
    LOOP UNIFORM_SPEED_COUNT
        uniform_point_base_index = uniform_move_index
        uniform_has_forward_phase = 0
        uniform_has_reverse_phase = 0
        uniform_cmd_speed = UNIFORM_SPEED_SET(uniform_i)
        uniform_cmd_acc = UNIFORM_ACC_SET(uniform_i)
        uniform_cmd_jerk = UNIFORM_JERK_SET(uniform_i)
        VEL(uniform_axis_no) = uniform_cmd_speed
        ACC(uniform_axis_no) = uniform_cmd_acc
        DEC(uniform_axis_no) = uniform_cmd_acc
        KDEC(uniform_axis_no) = uniform_cmd_acc
        JERK(uniform_axis_no) = uniform_cmd_jerk

        PTP uniform_axis_no, UNIFORM_LEFT_LIMIT
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 201
            goto uniform_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = uniform_point_base_index
        UNIFORM_MOVE_DIRECTION(uniform_point_base_index) = 1
        UNIFORM_MOVE_SPEED(uniform_point_base_index) = uniform_cmd_speed
        uniform_end_pos = UNIFORM_RIGHT_LIMIT
        PTP uniform_axis_no, uniform_end_pos
        TILL GPHASE(uniform_axis_no) = 4, UNIFORM_MOVE_TIMEOUT_MS
        if GPHASE(uniform_axis_no) = 4
            uniform_has_forward_phase = 1
            FILL(0, UNIFORM_DC_DATA)
            DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(uniform_axis_no), FVEL(uniform_axis_no)
            uniform_j = 0
            TILL ^S_ST.#DC, uniform_dc_collect_timeout_ms
            if S_DCN < UNIFORM_SAMPLE_COUNT
                STOPDC
                UNIFORM_ERROR = 206
                goto uniform_error
            end
        end
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 202
            goto uniform_error
        end
        if uniform_has_forward_phase
            LOOP UNIFORM_SAMPLE_COUNT
                UNIFORM_POS_DATA(uniform_point_base_index)(uniform_j) = UNIFORM_DC_DATA(0)(uniform_j)
                UNIFORM_VEL_DATA(uniform_point_base_index)(uniform_j) = UNIFORM_DC_DATA(1)(uniform_j)
                uniform_j = uniform_j + 1
            END
        end

        PTP uniform_axis_no, UNIFORM_RIGHT_LIMIT
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 203
            goto uniform_error
        end
        WAIT UNIFORM_SETTLE_MS

        UNIFORM_CUR_MOVE = uniform_point_base_index + 1
        UNIFORM_MOVE_DIRECTION(uniform_point_base_index + 1) = -1
        UNIFORM_MOVE_SPEED(uniform_point_base_index + 1) = uniform_cmd_speed
        uniform_end_pos = UNIFORM_LEFT_LIMIT
        PTP uniform_axis_no, uniform_end_pos
        TILL GPHASE(uniform_axis_no) = 4, UNIFORM_MOVE_TIMEOUT_MS
        if GPHASE(uniform_axis_no) = 4
            uniform_has_reverse_phase = 1
            FILL(0, UNIFORM_DC_DATA)
            DC UNIFORM_DC_DATA, UNIFORM_SAMPLE_COUNT, UNIFORM_SAMPLE_INTERVAL_MS, FPOS(uniform_axis_no), FVEL(uniform_axis_no)
            uniform_j = 0
            TILL ^S_ST.#DC, uniform_dc_collect_timeout_ms
            if S_DCN < UNIFORM_SAMPLE_COUNT
                STOPDC
                UNIFORM_ERROR = 208
                goto uniform_error
            end
        end
        TILL ^MST(uniform_axis_no).#MOVE, UNIFORM_MOVE_TIMEOUT_MS
        if MST(uniform_axis_no).#MOVE
            UNIFORM_ERROR = 204
            goto uniform_error
        end
        if uniform_has_reverse_phase
            LOOP UNIFORM_SAMPLE_COUNT
                UNIFORM_POS_DATA(uniform_point_base_index + 1)(uniform_j) = UNIFORM_DC_DATA(0)(uniform_j)
                UNIFORM_VEL_DATA(uniform_point_base_index + 1)(uniform_j) = UNIFORM_DC_DATA(1)(uniform_j)
                uniform_j = uniform_j + 1
            END
        end
        if uniform_has_forward_phase
            if uniform_has_reverse_phase
                uniform_move_index = uniform_move_index + 2
            end
        end
        uniform_i = uniform_i + 1
    END

    UNIFORM_MOVE_COUNT = uniform_move_index
    UNIFORM_DONE = 1
    disp "AXIS %d SPEED UNIFORMITY DONE, speed_count=%d, moves=%d, samples=%d", uniform_axis_no, UNIFORM_SPEED_COUNT, UNIFORM_MOVE_COUNT, UNIFORM_SAMPLE_COUNT
STOP

uniform_error:
    STOPDC
    HALT uniform_axis_no
    disp "AXIS %d SPEED UNIFORMITY FAILED, error=%d, move=%d", uniform_axis_no, UNIFORM_ERROR, UNIFORM_CUR_MOVE

AXIS_DYNAMIC_CAPABILITY_TEST:
    LOCAL INT dyncap_axis_no
    LOCAL INT dyncap_move_index
    LOCAL INT dyncap_j
    LOCAL REAL dyncap_start_pos
    LOCAL REAL dyncap_end_pos
    LOCAL REAL dyncap_dc_timeout_ms

    DYNCAP_DONE = 0
    DYNCAP_ERROR = 0
    DYNCAP_CUR_MOVE = -1
    DYNCAP_MOVE_COUNT = 0
    dyncap_axis_no = DYNCAP_AXIS

    if DYNCAP_SAMPLE_COUNT <= 0; DYNCAP_SAMPLE_COUNT = 5000 end
    if DYNCAP_SAMPLE_INTERVAL_MS <= 0; DYNCAP_SAMPLE_INTERVAL_MS = 1 end
    if DYNCAP_MOVE_TIMEOUT_MS <= 0; DYNCAP_MOVE_TIMEOUT_MS = 120000 end
    if DYNCAP_ENABLE_TIMEOUT_MS <= 0; DYNCAP_ENABLE_TIMEOUT_MS = 5000 end
    if DYNCAP_EDGE_MARGIN_MM <= 0; DYNCAP_EDGE_MARGIN_MM = 1 end
    if DYNCAP_SETTLE_MS <= 0; DYNCAP_SETTLE_MS = 100 end

    if DYNCAP_SAMPLE_COUNT > 10000
        DYNCAP_ERROR = 101
        goto dyncap_error
    end
    if DYNCAP_MAX_VEL <= 0
        DYNCAP_ERROR = 102
        goto dyncap_error
    end
    if DYNCAP_MAX_ACC <= 0
        DYNCAP_ERROR = 103
        goto dyncap_error
    end
    if DYNCAP_MAX_JERK <= 0
        DYNCAP_ERROR = 104
        goto dyncap_error
    end
    if DYNCAP_RIGHT_LIMIT <= DYNCAP_LEFT_LIMIT
        DYNCAP_LEFT_LIMIT = SLLIMIT(dyncap_axis_no)
        DYNCAP_RIGHT_LIMIT = SRLIMIT(dyncap_axis_no)
    end
    if DYNCAP_RIGHT_LIMIT <= DYNCAP_LEFT_LIMIT
        DYNCAP_ERROR = 105
        goto dyncap_error
    end

    dyncap_start_pos = DYNCAP_LEFT_LIMIT + DYNCAP_EDGE_MARGIN_MM
    dyncap_end_pos = DYNCAP_RIGHT_LIMIT - DYNCAP_EDGE_MARGIN_MM
    if dyncap_end_pos <= dyncap_start_pos
        DYNCAP_ERROR = 106
        goto dyncap_error
    end
    dyncap_dc_timeout_ms = (DYNCAP_SAMPLE_COUNT * DYNCAP_SAMPLE_INTERVAL_MS) + 2000

    FCLEAR dyncap_axis_no
    ENABLE dyncap_axis_no
    TILL MST(dyncap_axis_no).#ENABLED, DYNCAP_ENABLE_TIMEOUT_MS
    if ^MST(dyncap_axis_no).#ENABLED
        DYNCAP_ERROR = 108
        goto dyncap_error
    end

    VEL(dyncap_axis_no) = DYNCAP_MAX_VEL
    ACC(dyncap_axis_no) = DYNCAP_MAX_ACC
    DEC(dyncap_axis_no) = DYNCAP_MAX_ACC
    KDEC(dyncap_axis_no) = DYNCAP_MAX_ACC
    JERK(dyncap_axis_no) = DYNCAP_MAX_JERK

    dyncap_move_index = 0
    PTP dyncap_axis_no, dyncap_start_pos
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 201
        goto dyncap_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = dyncap_move_index
    DYNCAP_START_POS(dyncap_move_index) = dyncap_start_pos
    DYNCAP_END_POS(dyncap_move_index) = dyncap_end_pos
    DYNCAP_CMD_VEL(dyncap_move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(dyncap_move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(dyncap_move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(dyncap_move_index) = 1
    FILL(0, DYNCAP_DC_DATA)
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(dyncap_axis_no)
    PTP dyncap_axis_no, dyncap_end_pos
    dyncap_j = 0
    TILL ^S_ST.#DC, dyncap_dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 205
        goto dyncap_error
    end
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 202
        goto dyncap_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(dyncap_move_index)(dyncap_j) = DYNCAP_DC_DATA(0)(dyncap_j)
        dyncap_j = dyncap_j + 1
    END
    dyncap_move_index = dyncap_move_index + 1

    PTP dyncap_axis_no, dyncap_end_pos
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 203
        goto dyncap_error
    end
    WAIT DYNCAP_SETTLE_MS

    DYNCAP_CUR_MOVE = dyncap_move_index
    DYNCAP_START_POS(dyncap_move_index) = dyncap_end_pos
    DYNCAP_END_POS(dyncap_move_index) = dyncap_start_pos
    DYNCAP_CMD_VEL(dyncap_move_index) = DYNCAP_MAX_VEL
    DYNCAP_CMD_ACC(dyncap_move_index) = DYNCAP_MAX_ACC
    DYNCAP_CMD_JERK(dyncap_move_index) = DYNCAP_MAX_JERK
    DYNCAP_MOVE_DIRECTION(dyncap_move_index) = -1
    FILL(0, DYNCAP_DC_DATA)
    DC DYNCAP_DC_DATA, DYNCAP_SAMPLE_COUNT, DYNCAP_SAMPLE_INTERVAL_MS, FVEL(dyncap_axis_no)
    PTP dyncap_axis_no, dyncap_start_pos
    dyncap_j = 0
    TILL ^S_ST.#DC, dyncap_dc_timeout_ms
    if S_DCN < DYNCAP_SAMPLE_COUNT
        STOPDC
        DYNCAP_ERROR = 206
        goto dyncap_error
    end
    TILL ^MST(dyncap_axis_no).#MOVE, DYNCAP_MOVE_TIMEOUT_MS
    if MST(dyncap_axis_no).#MOVE
        DYNCAP_ERROR = 204
        goto dyncap_error
    end
    LOOP DYNCAP_SAMPLE_COUNT
        DYNCAP_VEL_DATA(dyncap_move_index)(dyncap_j) = DYNCAP_DC_DATA(0)(dyncap_j)
        dyncap_j = dyncap_j + 1
    END
    dyncap_move_index = dyncap_move_index + 1

    DYNCAP_MOVE_COUNT = dyncap_move_index
    DYNCAP_DONE = 1
    disp "AXIS %d DYNAMIC CAPABILITY DONE, moves=%d, samples=%d", dyncap_axis_no, DYNCAP_MOVE_COUNT, DYNCAP_SAMPLE_COUNT
STOP

dyncap_error:
    STOPDC
    HALT dyncap_axis_no
    disp "AXIS %d DYNAMIC CAPABILITY FAILED, error=%d, move=%d", dyncap_axis_no, DYNCAP_ERROR, DYNCAP_CUR_MOVE

AXIS_DROP_DISTANCE_TEST:
    LOCAL INT drop_axis_no
    LOCAL INT drop_i
    LOCAL REAL drop_pitch
    LOCAL REAL drop_prev_pos
    LOCAL REAL drop_curr_pos
    LOCAL REAL drop_stable_start

    DROP_DONE = 0
    DROP_ERROR = 0
    DROP_CUR_POINT = -1
    drop_axis_no = DROP_AXIS

    if DROP_POINT_COUNT <= 0; DROP_POINT_COUNT = 10 end
    if DROP_MOVE_TIMEOUT_MS <= 0; DROP_MOVE_TIMEOUT_MS = 60000 end
    if DROP_ENABLE_TIMEOUT_MS <= 0; DROP_ENABLE_TIMEOUT_MS = 5000 end
    if DROP_DISABLE_TIMEOUT_MS <= 0; DROP_DISABLE_TIMEOUT_MS = 5000 end
    if DROP_STOP_POLL_MS <= 0; DROP_STOP_POLL_MS = 20 end
    if DROP_STOP_WINDOW_MS <= 0; DROP_STOP_WINDOW_MS = 300 end
    if DROP_STOP_TOL <= 0; DROP_STOP_TOL = 0.001 end

    if DROP_POINT_COUNT > 20
        DROP_ERROR = 101
        goto drop_error
    end
    if DROP_RIGHT_LIMIT <= DROP_LEFT_LIMIT
        DROP_LEFT_LIMIT = SLLIMIT(drop_axis_no)
        DROP_RIGHT_LIMIT = SRLIMIT(drop_axis_no)
    end
    if DROP_RIGHT_LIMIT <= DROP_LEFT_LIMIT
        DROP_ERROR = 102
        goto drop_error
    end

    drop_pitch = (DROP_RIGHT_LIMIT - DROP_LEFT_LIMIT) / (DROP_POINT_COUNT + 1)
    drop_i = 0
    LOOP DROP_POINT_COUNT
        DROP_TARGET_POS(drop_i) = DROP_LEFT_LIMIT + drop_pitch * (drop_i + 1)
        drop_i = drop_i + 1
    END

    FCLEAR drop_axis_no
    ENABLE drop_axis_no
    TILL MST(drop_axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
    if ^MST(drop_axis_no).#ENABLED
        DROP_ERROR = 103
        goto drop_error
    end

    drop_i = 0
    LOOP DROP_POINT_COUNT
        DROP_CUR_POINT = drop_i
        PTP drop_axis_no, DROP_TARGET_POS(drop_i)
        TILL ^MST(drop_axis_no).#MOVE, DROP_MOVE_TIMEOUT_MS
        if MST(drop_axis_no).#MOVE
            DROP_ERROR = 201
            goto drop_error
        end

        WAIT 100
        DROP_PRE_DISABLE_POS(drop_i) = FPOS(drop_axis_no)
        DISABLE drop_axis_no
        TILL ^MST(drop_axis_no).#ENABLED, DROP_DISABLE_TIMEOUT_MS
        if MST(drop_axis_no).#ENABLED
            DROP_ERROR = 202
            goto drop_error
        end

        drop_prev_pos = FPOS(drop_axis_no)
        drop_stable_start = TIME
        while (TIME - drop_stable_start) < DROP_STOP_WINDOW_MS
            WAIT DROP_STOP_POLL_MS
            drop_curr_pos = FPOS(drop_axis_no)
            if ABS(drop_curr_pos - drop_prev_pos) > DROP_STOP_TOL
                drop_stable_start = TIME
            end
            drop_prev_pos = drop_curr_pos
        end

        DROP_POST_DROP_POS(drop_i) = drop_prev_pos
        DROP_DISTANCE(drop_i) = DROP_POST_DROP_POS(drop_i) - DROP_PRE_DISABLE_POS(drop_i)
        FCLEAR drop_axis_no
        ENABLE drop_axis_no
        TILL MST(drop_axis_no).#ENABLED, DROP_ENABLE_TIMEOUT_MS
        if ^MST(drop_axis_no).#ENABLED
            DROP_ERROR = 203
            goto drop_error
        end
        drop_i = drop_i + 1
    END

    DROP_DONE = 1
    disp "AXIS %d DROP DISTANCE DONE, points=%d", drop_axis_no, DROP_POINT_COUNT
STOP

drop_error:
    HALT drop_axis_no
    disp "AXIS %d DROP DISTANCE FAILED, error=%d, point=%d", drop_axis_no, DROP_ERROR, DROP_CUR_POINT

#A
!PNAME=5 X2Home
!PDESC=
!axisdef X=0,Y=1,Z=2,T=3,A=4,B=5,C=6,D=7
!axisdef x=0,y=1,z=2,t=3,a=4,b=5,c=6,d=7
global int I(100),I0,I1,I2,I3,I4,I5,I6,I7,I8,I9,I90,I91,I92,I93,I94,I95,I96,I97,I98,I99
global real V(100),V0,V1,V2,V3,V4,V5,V6,V7,V8,V9,V90,V91,V92,V93,V94,V95,V96,V97,V98,V99

global int action_done
