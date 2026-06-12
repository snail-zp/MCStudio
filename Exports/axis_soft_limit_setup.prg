!
! Axis electrical/software limit setup
! Usage:
! 1. Set LIMIT_AXIS to the target axis number.
! 2. Optional: set LIMIT_MARGIN_MM and LIMIT_SCAN_VEL.
! 3. Run AXIS_SOFT_LIMIT_SETUP.
!
! Result:
! - LIMIT_ELEC_NEG / LIMIT_ELEC_POS : detected electrical limits
! - LIMIT_SOFT_NEG / LIMIT_SOFT_POS : software limits after 1mm inward offset
! - LIMIT_STROKE                    : software stroke
!

INT BUFFER_ID; BUFFER_ID = 31

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

AXIS_SOFT_LIMIT_SETUP:
    LOCAL INT axis_no

    LIMIT_DONE = 0
    LIMIT_ERROR = 0

    axis_no = LIMIT_AXIS

    if LIMIT_MARGIN_MM <= 0
        LIMIT_MARGIN_MM = 1
    end

    if LIMIT_SCAN_VEL <= 0
        LIMIT_SCAN_VEL = 5
    end

    if axis_no < 0
        LIMIT_ERROR = 101
        goto limit_error
    end

    FDEF(axis_no).#RL = 0
    FDEF(axis_no).#LL = 0
    FMASK(axis_no).#SRL = 0
    FMASK(axis_no).#SLL = 0

    FCLEAR axis_no
    if ^MST(axis_no).#ENABLED
        ENABLE axis_no
        TILL MST(axis_no).#ENABLED, 5000
        if ^MST(axis_no).#ENABLED
            LIMIT_ERROR = 102
            goto limit_restore
        end
    end

    JOG/V axis_no, LIMIT_SCAN_VEL
    TILL FAULT(axis_no).#RL, 120000
    if ^FAULT(axis_no).#RL
        LIMIT_ERROR = 201
        goto limit_restore
    end

    JOG/V axis_no, -LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(axis_no).#RL, 10000
    HALT axis_no
    WAIT 100

    LIMIT_ELEC_POS = FPOS(axis_no)
    LIMIT_SOFT_POS = LIMIT_ELEC_POS - LIMIT_MARGIN_MM
    SRLIMIT(axis_no) = LIMIT_SOFT_POS

    FCLEAR axis_no
    JOG/V axis_no, -LIMIT_SCAN_VEL
    TILL FAULT(axis_no).#LL, 120000
    if ^FAULT(axis_no).#LL
        LIMIT_ERROR = 202
        goto limit_restore
    end

    JOG/V axis_no, LIMIT_SCAN_VEL * 0.2
    TILL ^FAULT(axis_no).#LL, 10000
    HALT axis_no
    WAIT 100

    LIMIT_ELEC_NEG = FPOS(axis_no)
    LIMIT_SOFT_NEG = LIMIT_ELEC_NEG + LIMIT_MARGIN_MM
    SLLIMIT(axis_no) = LIMIT_SOFT_NEG

    LIMIT_STROKE = LIMIT_SOFT_POS - LIMIT_SOFT_NEG

    FCLEAR axis_no
    ENABLE axis_no
    PTP/E axis_no, (LIMIT_SOFT_NEG + LIMIT_SOFT_POS) / 2

    LIMIT_DONE = 1
    disp "AXIS %d LIMITS: elec_neg=%.4f elec_pos=%.4f soft_neg=%.4f soft_pos=%.4f stroke=%.4f", axis_no, LIMIT_ELEC_NEG, LIMIT_ELEC_POS, LIMIT_SOFT_NEG, LIMIT_SOFT_POS, LIMIT_STROKE

limit_restore:
    FDEF(axis_no).#RL = 1
    FDEF(axis_no).#LL = 1
    FMASK(axis_no).#SRL = 1
    FMASK(axis_no).#SLL = 1
    if LIMIT_DONE
        STOP
    end

limit_error:
    HALT axis_no
    disp "AXIS %d LIMIT SETUP FAILED, error=%d", axis_no, LIMIT_ERROR
    goto limit_restore
