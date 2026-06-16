!
! Scan-area PEG raster buffer
!
! Host writes a rectangular scan area. This buffer adds lead-in / lead-out
! extension so the rectangle is scanned in the constant-speed segment.
!
! Main label:
! - ScanArea_Run
!
! Function labels:
! - ScanArea_Reset
! - Scan_Validate_Config
! - Scan_Calc_Extension
! - Scan_Build_Row_Table
! - Scan_Set_Motion_Profile
! - Scan_Move_To_First_Row
! - Scan_Prepare_Row
! - Scan_Move_To_Row_Start
! - Scan_Peg_Arm_All_Rows
! - Scan_Peg_Arm_Row
! - Scan_Peg_Stop_All
! - Scan_Build_PVSpline_Path
! - Scan_Execute_PVSpline_Path
!
! PEG note:
! The PEG command format depends on the controller / PEG hardware assignment.
! PEG switching is intentionally separated from motion. Replace the TODO
! blocks in Scan_Peg_Arm_Row and Scan_Peg_Stop_All with your final ACS PEG
! commands.
!

GLOBAL INT SCAN_AXIS_X
GLOBAL INT SCAN_AXIS_Y
GLOBAL INT SCAN_DONE
GLOBAL INT SCAN_ERROR
GLOBAL INT SCAN_CUR_ROW
GLOBAL INT SCAN_ROW_COUNT
GLOBAL INT SCAN_DIRECTION
GLOBAL INT SCAN_PVS_POINT_COUNT

GLOBAL REAL SCAN_X_MIN
GLOBAL REAL SCAN_X_MAX
GLOBAL REAL SCAN_Y_MIN
GLOBAL REAL SCAN_Y_MAX
GLOBAL REAL SCAN_ROW_PITCH
GLOBAL REAL SCAN_VEL
GLOBAL REAL SCAN_ACC
GLOBAL REAL SCAN_JERK
GLOBAL REAL SCAN_EXTEND_MIN
GLOBAL REAL SCAN_EXTEND_MARGIN
GLOBAL REAL SCAN_LINE_CHANGE_SETTLE_MS
GLOBAL REAL SCAN_MOVE_TIMEOUT_MS

GLOBAL REAL SCAN_EXTENSION
GLOBAL REAL SCAN_ROW_Y
GLOBAL REAL SCAN_LINE_START
GLOBAL REAL SCAN_LINE_END
GLOBAL REAL SCAN_ENTER_POS
GLOBAL REAL SCAN_EXIT_POS
GLOBAL REAL SCAN_ACCEL_DISTANCE
GLOBAL REAL SCAN_JERK_TIME
GLOBAL REAL SCAN_CONST_ACC_TIME
GLOBAL REAL SCAN_WAIT_START

GLOBAL INT SCAN_PEG_ENTER_OUTPUT
GLOBAL INT SCAN_PEG_EXIT_OUTPUT
GLOBAL INT SCAN_PEG_ENTER_CHANNEL
GLOBAL INT SCAN_PEG_EXIT_CHANNEL
GLOBAL REAL SCAN_PEG_WIDTH_US
GLOBAL INT SCAN_PEG_ACTIVE
GLOBAL REAL SCAN_PVS_INTERVAL_MS

GLOBAL REAL SCAN_ROW_START_X(512)
GLOBAL REAL SCAN_ROW_END_X(512)
GLOBAL REAL SCAN_ROW_Y_POS(512)
GLOBAL REAL SCAN_PVS_PATH(4)(1024)

ScanArea_Reset:
    SCAN_DONE = 0
    SCAN_ERROR = 0
    SCAN_CUR_ROW = 0
    SCAN_DIRECTION = 1
    SCAN_PEG_ACTIVE = 0

    if SCAN_AXIS_X < 0
        SCAN_AXIS_X = 0
    end
    if SCAN_AXIS_Y < 0
        SCAN_AXIS_Y = 1
    end
    if SCAN_ROW_PITCH <= 0
        SCAN_ROW_PITCH = 1
    end
    if SCAN_VEL <= 0
        SCAN_VEL = 50
    end
    if SCAN_ACC <= 0
        SCAN_ACC = SCAN_VEL * 10
    end
    if SCAN_JERK <= 0
        SCAN_JERK = SCAN_ACC
    end
    if SCAN_EXTEND_MARGIN < 0
        SCAN_EXTEND_MARGIN = 0
    end
    if SCAN_LINE_CHANGE_SETTLE_MS <= 0
        SCAN_LINE_CHANGE_SETTLE_MS = 20
    end
    if SCAN_MOVE_TIMEOUT_MS <= 0
        SCAN_MOVE_TIMEOUT_MS = 30000
    end
    if SCAN_PEG_WIDTH_US <= 0
        SCAN_PEG_WIDTH_US = 10
    end
    if SCAN_PVS_INTERVAL_MS <= 0
        SCAN_PVS_INTERVAL_MS = 20
    end
STOP

ScanArea_Run:
    SCAN_DONE = 0
    SCAN_ERROR = 0
    SCAN_CUR_ROW = 0
    SCAN_PEG_ACTIVE = 0

    CALL Scan_Validate_Config
    if SCAN_ERROR <> 0
        goto Scan_Error
    end

    CALL Scan_Calc_Extension
    CALL Scan_Build_Row_Table
    CALL Scan_Set_Motion_Profile
    CALL Scan_Move_To_First_Row
    if SCAN_ERROR <> 0
        goto Scan_Error
    end

    CALL Scan_Build_PVSpline_Path
    if SCAN_ERROR <> 0
        goto Scan_Error
    end

    CALL Scan_Peg_Arm_All_Rows
    if SCAN_ERROR <> 0
        goto Scan_Error
    end

    CALL Scan_Execute_PVSpline_Path
    if SCAN_ERROR <> 0
        goto Scan_Error
    end

    CALL Scan_Peg_Stop_All
    SCAN_DONE = 1
STOP

Scan_Validate_Config:
    if SCAN_X_MAX <= SCAN_X_MIN
        SCAN_ERROR = 101
        RET
    end
    if SCAN_Y_MAX < SCAN_Y_MIN
        SCAN_ERROR = 102
        RET
    end
    if SCAN_ROW_PITCH <= 0
        SCAN_ERROR = 103
        RET
    end
    if SCAN_VEL <= 0
        SCAN_ERROR = 104
        RET
    end
    if SCAN_ACC <= 0
        SCAN_ERROR = 105
        RET
    end
    if SCAN_JERK <= 0
        SCAN_ERROR = 106
        RET
    end
    if SCAN_PVS_INTERVAL_MS <= 0
        SCAN_ERROR = 107
        RET
    end
    SCAN_ERROR = 0
RET

Scan_Calc_Extension:
    !
    ! ACS segmented-motion examples calculate the distance needed to change
    ! velocity with jerk limiting as:
    !   TJ = ACC / JERK
    !   TA = (DeltaV / ACC) - TJ
    !   if TA < 0: TA = 0, TJ = SQRT(DeltaV / JERK)
    !   P = (Vstart + Vend) * (TA / 2 + TJ)
    !
    ! Here Vstart=0 and Vend=SCAN_VEL. This handles both cases:
    ! - trapezoidal / S curve with a constant-acceleration part
    ! - triangular S curve with no constant-acceleration part
    !
    SCAN_JERK_TIME = SCAN_ACC / SCAN_JERK
    SCAN_CONST_ACC_TIME = (SCAN_VEL / SCAN_ACC) - SCAN_JERK_TIME

    if SCAN_CONST_ACC_TIME < 0
        SCAN_CONST_ACC_TIME = 0
        SCAN_JERK_TIME = SQRT(SCAN_VEL / SCAN_JERK)
    end

    SCAN_ACCEL_DISTANCE = SCAN_VEL * ((SCAN_CONST_ACC_TIME / 2) + SCAN_JERK_TIME)
    SCAN_EXTENSION = SCAN_ACCEL_DISTANCE + SCAN_EXTEND_MARGIN

    if SCAN_EXTENSION < SCAN_EXTEND_MIN
        SCAN_EXTENSION = SCAN_EXTEND_MIN
    end
    if SCAN_EXTENSION <= 0
        SCAN_EXTENSION = SCAN_ROW_PITCH
    end

    SCAN_ROW_COUNT = ((SCAN_Y_MAX - SCAN_Y_MIN) / SCAN_ROW_PITCH) + 1
    if SCAN_ROW_COUNT < 1
        SCAN_ROW_COUNT = 1
    end
    if SCAN_ROW_COUNT > 512
        SCAN_ROW_COUNT = 512
    end
RET

Scan_Build_Row_Table:
    INT row
    row = 0

    while row < SCAN_ROW_COUNT
        SCAN_ROW_Y_POS(row) = SCAN_Y_MIN + (row * SCAN_ROW_PITCH)
        if SCAN_ROW_Y_POS(row) > SCAN_Y_MAX
            SCAN_ROW_Y_POS(row) = SCAN_Y_MAX
        end

        if (row % 2) = 0
            SCAN_ROW_START_X(row) = SCAN_X_MIN - SCAN_EXTENSION
            SCAN_ROW_END_X(row) = SCAN_X_MAX + SCAN_EXTENSION
        else
            SCAN_ROW_START_X(row) = SCAN_X_MAX + SCAN_EXTENSION
            SCAN_ROW_END_X(row) = SCAN_X_MIN - SCAN_EXTENSION
        end
        row = row + 1
    end
RET

Scan_Set_Motion_Profile:
    VEL(SCAN_AXIS_X) = SCAN_VEL
    ACC(SCAN_AXIS_X) = SCAN_ACC
    DEC(SCAN_AXIS_X) = SCAN_ACC
    KDEC(SCAN_AXIS_X) = SCAN_ACC
    JERK(SCAN_AXIS_X) = SCAN_JERK

    VEL(SCAN_AXIS_Y) = SCAN_VEL
    ACC(SCAN_AXIS_Y) = SCAN_ACC
    DEC(SCAN_AXIS_Y) = SCAN_ACC
    KDEC(SCAN_AXIS_Y) = SCAN_ACC
    JERK(SCAN_AXIS_Y) = SCAN_JERK
RET

Scan_Move_To_First_Row:
    PTP SCAN_AXIS_X, SCAN_ROW_START_X(0)
    CALL Scan_Wait_X_Inpos
    if SCAN_ERROR <> 0
        RET
    end

    PTP SCAN_AXIS_Y, SCAN_ROW_Y_POS(0)
    CALL Scan_Wait_Y_Inpos
    WAIT SCAN_LINE_CHANGE_SETTLE_MS
RET

Scan_Prepare_Row:
    SCAN_ROW_Y = SCAN_ROW_Y_POS(SCAN_CUR_ROW)
    SCAN_LINE_START = SCAN_ROW_START_X(SCAN_CUR_ROW)
    SCAN_LINE_END = SCAN_ROW_END_X(SCAN_CUR_ROW)

    if SCAN_LINE_END >= SCAN_LINE_START
        SCAN_DIRECTION = 1
        SCAN_ENTER_POS = SCAN_X_MIN
        SCAN_EXIT_POS = SCAN_X_MAX
    else
        SCAN_DIRECTION = -1
        SCAN_ENTER_POS = SCAN_X_MAX
        SCAN_EXIT_POS = SCAN_X_MIN
    end
RET

Scan_Move_To_Row_Start:
    PTP SCAN_AXIS_X, SCAN_LINE_START
    CALL Scan_Wait_X_Inpos
    if SCAN_ERROR <> 0
        RET
    end

    PTP SCAN_AXIS_Y, SCAN_ROW_Y
    CALL Scan_Wait_Y_Inpos
    WAIT SCAN_LINE_CHANGE_SETTLE_MS
RET

Scan_Peg_Arm_All_Rows:
    CALL Scan_Peg_Stop_All

    SCAN_CUR_ROW = 0
    while SCAN_CUR_ROW < SCAN_ROW_COUNT
        CALL Scan_Prepare_Row
        CALL Scan_Peg_Arm_Row
        if SCAN_ERROR <> 0
            RET
        end
        SCAN_CUR_ROW = SCAN_CUR_ROW + 1
    end

    SCAN_PEG_ACTIVE = 1
RET

Scan_Peg_Arm_Row:
    !
    ! TODO: configure two hardware PEG events for SCAN_CUR_ROW only.
    ! This label contains PEG configuration only. Do not place motion commands
    ! here; Scan_Execute_PVSpline_Path owns all movement.
    !
    ! Available parameters:
    ! - axis / encoder   : SCAN_AXIS_X
    ! - row y            : SCAN_ROW_Y
    ! - enter position   : SCAN_ENTER_POS
    ! - exit position    : SCAN_EXIT_POS
    ! - direction        : SCAN_DIRECTION
    ! - enter output     : SCAN_PEG_ENTER_OUTPUT
    ! - exit output      : SCAN_PEG_EXIT_OUTPUT
    ! - enter channel    : SCAN_PEG_ENTER_CHANNEL
    ! - exit channel     : SCAN_PEG_EXIT_CHANNEL
    ! - pulse width      : SCAN_PEG_WIDTH_US
    !
    ! Example placeholder only:
    ! PEG_ENTER_CONFIG_HERE
    ! PEG_EXIT_CONFIG_HERE
    !
RET

Scan_Build_PVSpline_Path:
    SCAN_PVS_POINT_COUNT = 0
    SCAN_CUR_ROW = 0

    while SCAN_CUR_ROW < SCAN_ROW_COUNT
        if SCAN_PVS_POINT_COUNT + 4 > 1024
            SCAN_ERROR = 301
            RET
        end

        CALL Scan_Prepare_Row

        ! Lead-in start. The axis accelerates before it reaches SCAN_ENTER_POS.
        SCAN_PVS_PATH(0)(SCAN_PVS_POINT_COUNT) = SCAN_LINE_START
        SCAN_PVS_PATH(1)(SCAN_PVS_POINT_COUNT) = SCAN_ROW_Y
        SCAN_PVS_PATH(2)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_PATH(3)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_POINT_COUNT = SCAN_PVS_POINT_COUNT + 1

        ! Scan-area enter point. PEG enter event should be armed here.
        SCAN_PVS_PATH(0)(SCAN_PVS_POINT_COUNT) = SCAN_ENTER_POS
        SCAN_PVS_PATH(1)(SCAN_PVS_POINT_COUNT) = SCAN_ROW_Y
        SCAN_PVS_PATH(2)(SCAN_PVS_POINT_COUNT) = SCAN_DIRECTION * SCAN_VEL
        SCAN_PVS_PATH(3)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_POINT_COUNT = SCAN_PVS_POINT_COUNT + 1

        ! Scan-area exit point. The rectangle is inside the constant-speed span.
        SCAN_PVS_PATH(0)(SCAN_PVS_POINT_COUNT) = SCAN_EXIT_POS
        SCAN_PVS_PATH(1)(SCAN_PVS_POINT_COUNT) = SCAN_ROW_Y
        SCAN_PVS_PATH(2)(SCAN_PVS_POINT_COUNT) = SCAN_DIRECTION * SCAN_VEL
        SCAN_PVS_PATH(3)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_POINT_COUNT = SCAN_PVS_POINT_COUNT + 1

        ! Lead-out end. The next row starts at the same X on another Y, so the
        ! line-change is still part of the same PVSPLINE path.
        SCAN_PVS_PATH(0)(SCAN_PVS_POINT_COUNT) = SCAN_LINE_END
        SCAN_PVS_PATH(1)(SCAN_PVS_POINT_COUNT) = SCAN_ROW_Y
        SCAN_PVS_PATH(2)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_PATH(3)(SCAN_PVS_POINT_COUNT) = 0
        SCAN_PVS_POINT_COUNT = SCAN_PVS_POINT_COUNT + 1

        SCAN_CUR_ROW = SCAN_CUR_ROW + 1
    end

    if SCAN_PVS_POINT_COUNT < 2
        SCAN_ERROR = 302
    end
RET

Scan_Execute_PVSpline_Path:
    PVSPLINE (SCAN_AXIS_X, SCAN_AXIS_Y), SCAN_PVS_INTERVAL_MS
    MPOINT (SCAN_AXIS_X, SCAN_AXIS_Y), SCAN_PVS_PATH, SCAN_PVS_POINT_COUNT
    ENDS (SCAN_AXIS_X, SCAN_AXIS_Y)
    CALL Scan_Wait_X_Inpos
    if SCAN_ERROR <> 0
        RET
    end
    CALL Scan_Wait_Y_Inpos
RET

Scan_Peg_Stop_All:
    !
    ! TODO: stop / clear both PEG channels.
    ! This label contains PEG cleanup only. Motion cleanup belongs to caller.
    !
    ! Example placeholder only:
    ! PEG_STOP_ENTER_CHANNEL_HERE
    ! PEG_STOP_EXIT_CHANNEL_HERE
    !
    SCAN_PEG_ACTIVE = 0
RET

Scan_Wait_X_Inpos:
    SCAN_WAIT_START = TIME
    while MST(SCAN_AXIS_X).#INPOS = 0
        if (TIME - SCAN_WAIT_START) > SCAN_MOVE_TIMEOUT_MS
            SCAN_ERROR = 201
            RET
        end
        WAIT 5
    end
RET

Scan_Wait_Y_Inpos:
    SCAN_WAIT_START = TIME
    while MST(SCAN_AXIS_Y).#INPOS = 0
        if (TIME - SCAN_WAIT_START) > SCAN_MOVE_TIMEOUT_MS
            SCAN_ERROR = 202
            RET
        end
        WAIT 5
    end
RET

Scan_Error:
    CALL Scan_Peg_Stop_All
    SCAN_DONE = -1
STOP
