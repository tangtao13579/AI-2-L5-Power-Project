#include "GlobalDefine.h"

#define TASK_PERIOD 10  /*ms*/

extern GlobalVariableDef GlobalVariable;

static void (*WorkModeFunction)(void);
static void (*AutoModeFunction)(void);


/**********************************************************************************************************************
                                        Private Function
***********************************************************************************************************************/

static void WindCheck(void);
static void BatSOCLowCheck(void);

static void IdelMode(void);

static void ManualMode(void);

static void MaintenanceMode(void);
static void AngleCalibrationMode(void);

static void AutoWindMode(void);
static void AutoBatSOCLowMode(void);
static void AutoAIMode(void);
static void AutoRainMode(void);
static void AutoSnowMode(void);
static void AutoTrackMode(void);
static void AutoModeSwitch(void);
static void AutoMode(void);


static void WindCheck()
{

}
static void BatSOCLowCheck()
{
    if((GlobalVariable.WorkMode.WorkMode & 0xFF) != AUTO_WIND_MODE)
    {
        if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_BATSOCLOW_MODE) /*BATSOCLOW */
        {
            if(GlobalVariable.WarningAndFault.BatSOCLow == 0)
            {
                GlobalVariable.WorkMode.WorkMode = 0x00000000 | AUTO_MODE;
            }
        }
        else
        {
            if(GlobalVariable.WarningAndFault.BatSOCLow != 0)
            {
                GlobalVariable.WorkMode.WorkMode = 0x00000000 | AUTO_BATSOCLOW_MODE;
            }
        }
    }
    
}


static void IdelMode()
{
    GlobalVariable.Motor[0].MotorEnable = 0;
    if(GlobalVariable.WorkMode.WorkMode == 0xFF)
    {
        GlobalVariable.WorkMode.WorkMode = 0x00;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xF0) == AUTO_MODE)
    {
        WorkModeFunction = AutoMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xF0) == MANUAL_MODE)
    {
        WorkModeFunction = ManualMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xF0) == MAINTENANCE_MODE)
    {
        WorkModeFunction = MaintenanceMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xF0) == ANGLE_CALIBRATION_MODE)
    {
        WorkModeFunction = AngleCalibrationMode;
    }
    else
    {
        GlobalVariable.WorkMode.WorkMode = 0x00;
        GlobalVariable.WorkMode.Target = GlobalVariable.AstronomyPara.AstronomicalTargetAngle;
    }
}
static void ManualMode()
{

}

static void MaintenanceMode()
{

}

static void AngleCalibrationMode()
{

}
static void AutoWindMode()
{

}
static void AutoBatSOCLowMode()
{
    if((GlobalVariable.WorkMode.WorkMode & 0xFF) != AUTO_BATSOCLOW_MODE)
    {
        AutoModeFunction = AutoModeSwitch;
        return;
    }
    GlobalVariable.Motor[0].NeedLeadAngle = 0;
    GlobalVariable.WorkMode.Target = 0;//(float)GlobalVariable.ConfigPara.BackAngle;这里有个问题没有角度
    GlobalVariable.Motor[0].MotorEnable = 1;
}
static void AutoAIMode()
{

}
static void AutoRainMode()
{

}
static void AutoSnowMode()
{

}
static void AutoTrackMode()
{
 
}
static void AutoModeSwitch()
{
    GlobalVariable.Motor[0].MotorEnable = 0;
    
    if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_WIND_MODE)
    {
        AutoModeFunction = AutoWindMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_BATSOCLOW_MODE)
    {
        AutoModeFunction = AutoBatSOCLowMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_AI_MODE)
    {
        AutoModeFunction = AutoAIMode;
        GlobalVariable.AIPara.AIModeDelayCount = 0;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_RAIN_MODE)
    {
        AutoModeFunction = AutoRainMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_SNOW_MODE)
    {
        AutoModeFunction = AutoSnowMode;
    }
    else if((GlobalVariable.WorkMode.WorkMode & 0xFF) == AUTO_MODE)
    {
        AutoModeFunction = AutoTrackMode;
    }
    else
    {
        WorkModeFunction = IdelMode;
    }
}

static void AutoMode()
{
    if((GlobalVariable.WorkMode.WorkMode & 0xF0) == AUTO_MODE)
    {
        if(GlobalVariable.WarningAndFault.TimeLost == 0
         &&GlobalVariable.WarningAndFault.RTCError == 0)
        {
            /*Mode Decide*/
            /*WindCheck*/
            WindCheck();
            /*BatSOClowCheck*/
            BatSOCLowCheck();
            /*AutoModeRun*/
            (*AutoModeFunction)();
        }
        else
        {
            GlobalVariable.Motor[0].MotorEnable = 0;
        }
    }
    else
    {
        WorkModeFunction = IdelMode;
        GlobalVariable.Motor[0].MotorEnable = 0;
    }
}

/***********************************************************************************************************************
                                            Public function
***********************************************************************************************************************/
void WorkModeInit()
{
    AutoModeFunction = AutoModeSwitch;
    WorkModeFunction = IdelMode;
}
void WorkModeMg()
{
    (*WorkModeFunction)();
}

