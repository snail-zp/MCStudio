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
