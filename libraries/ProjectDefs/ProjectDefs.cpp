/*
 * ProjectDefs.cpp - Some common funcitons for SmartUpsEmulator.ino and SmtUpsEm_FactoryInit.ino
 *
 *
 *
 */

#include "ProjectDefs.h"	// For our Smart UPS Emulator project. Defs SERIAL1_DEBUG

void FactoryDefault(void)
{
    EEPROM.get(EEP_OFFSET_FACTORY, StoreEE); // Fetch our structure of non-volitale vars from "Factory Default" EEPROM
}

const BatteryParams bp_AGM = {
    1050,   0,          // VtoCapacitiesHiDisch[NUM_CAPACITY_POINTS];
    1151,  10,          //
    1275,  90,          //
    1282, 100,          //

    1100,   0,          // VtoCapacitiesLoDisch[NUM_CAPACITY_POINTS];
    1201,  10,          //
    1325,  90,          //
    1332, 100,          //

    1100+50,   0,          // VtoCapacitiesChrg[NUM_CAPACITY_POINTS];
    1201+50,  10,          //
    1325+50,  90,          //
    1332+50, 100,          //

    1380,                // batFullVoltage;
    1250,                // warningVoltage;
    1240,                // shutdownVoltage;
    1280,                // isChargingVolts;
    1150,                // isDisChargingVolts;
    (120 * 60),          // iCalcdTimeToEmpty;
    60 / 4,                // hiRateDichargeMins: 4C rate of discharge
    60 * 2,                // loRateDichargeMins: 0.5C rate of discharge
    0                    // timeToEmptyCalcState;              
    };                               
    

void FactoryCompiledDefault(void)
{
    //StoreEE.debugFlags        =  0;  // No debug prints by default  // DBC.007
    StoreEE.debugFlags        =  0;  // Debug prints by default       // DBC.007
    StoreEE.eeValid_1         = EEPROM_VALID_PAT1;      // Set sig in case user stores config to EEPROM.

    StoreEE.eeValid_2         = EEPROM_VALID_PAT2;
    StoreEE.eeVersion         = EEPROM_END_VER_SIG;


    // Physical parameters
    StoreEE.iAvgTimeToFull     = 120*60;
    StoreEE.iAvgTimeToEmpty    = 120*60;
    StoreEE.iRemainTimeLimit   =  5*60;
    StoreEE.reportIntervalSecs =  10;

    StoreEE.BatChem            = BC_AGM;
    StoreEE.battSysVMultiplier = 1;     // 1:12V
    StoreEE.reportingHysteresis = 2;    // 8 mV
    

    // Lead-Acid
    StoreEE.BattParams[BC_LeadAcid].batFullVoltage     = 1288;  // Voltage in hundredths (V*100), Bat Full
    StoreEE.BattParams[BC_LeadAcid].warningVoltage     = 1250;  // V*100, Bat Empty       
    StoreEE.BattParams[BC_LeadAcid].shutdownVoltage    = 1140;  // V*100, Bat Empty       
    StoreEE.BattParams[BC_LeadAcid].isChargingVolts    = 1288;  // V*100, Above this, must be charging
    StoreEE.BattParams[BC_LeadAcid].isDisChargingVolts = 1170;  // V*100, Below this, must be discharging
    //StoreEE.BattParams[BC_LeadAcid].numCapacityPointsUsed = NUM_CAPACITY_POINTS;    // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LeadAcid].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LeadAcid].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: PbAcid
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[0].voltage = 1162;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[0].capacity =  0;    // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[1].voltage = 1170;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[2].voltage = 1278;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[2].capacity =  90;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[3].voltage = 1288;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesHiDisch[3].capacity = 100;   // 

    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[0].voltage = 1162+50;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[0].capacity =  0;    // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[1].voltage = 1170+50;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[2].voltage = 1278+50;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[2].capacity =  90;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[3].voltage = 1288+50;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesLoDisch[3].capacity = 100;   // 

    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[0].voltage = 1162+100;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[0].capacity =  0;    // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[1].voltage = 1170+100;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[2].voltage = 1278+100;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[2].capacity =  90;   // 
    StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[3].voltage = 1288+100;  StoreEE.BattParams[BC_LeadAcid].VtoCapacitiesChrg[3].capacity = 100;   // 

    StoreEE.BattParams[BC_LeadAcid].hiRateDichargeMins = 60 / 4; // hiRateDichargeMins: 4C rate of discharge   ;
    StoreEE.BattParams[BC_LeadAcid].loRateDichargeMins = 60 * 2; // loRateDichargeMins: 0.5C rate of discharge ;


#if false
    // AGM
    StoreEE.BattParams[BC_AGM].batFullVoltage     = 1380;   // Voltage in hundredths (V*100), Bat Full
    StoreEE.BattParams[BC_AGM].shutdownVoltage    = 1050;   // V*100, Bat Empty       
    StoreEE.BattParams[BC_AGM].isChargingVolts    = 1280;   // V*100, Above this, must be charging
    StoreEE.BattParams[BC_AGM].isDisChargingVolts = 1150;   // V*100, Below this, must be discharging
    StoreEE.BattParams[BC_AGM].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_AGM].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_AGM].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: AGM
    StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[0].voltage = 1050;  StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[0].capacity =  0;    // 
    StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[1].voltage = 1151;  StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[1].capacity =  10;   // 
    StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[2].voltage = 1275;  StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[2].capacity =  90;   // 
    StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[3].voltage = 1282;  StoreEE.BattParams[BC_AGM].VtoCapacitiesHiDisch[3].capacity = 100;   // 
#else
    // This saves 78 bytes of program space, takes up 28 more bytes of RAM (just for BC_AGM)
    memcpy(&StoreEE.BattParams[BC_AGM], &bp_AGM, sizeof(StoreEE.BattParams[BC_AGM]));
#endif

    // LI_ION
    StoreEE.BattParams[BC_LI_ION].batFullVoltage     = 1360;   // Voltage in hundredths (V*100), Bat Full 
    StoreEE.BattParams[BC_LI_ION].warningVoltage     = 1250;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LI_ION].shutdownVoltage    = 1240;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LI_ION].isChargingVolts    = 1365;   // V*100, Above this, must be charging     
    StoreEE.BattParams[BC_LI_ION].isDisChargingVolts = 1200;   // V*100, Below this, must be discharging  
    //StoreEE.BattParams[BC_LI_ION].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LI_ION].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LI_ION].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: Li-Ion
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[0].voltage = 1000;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[1].voltage = 1200;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[2].voltage = 1280;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[3].voltage = 1360;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesHiDisch[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[0].voltage = 1000+50;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[1].voltage = 1200+50;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[2].voltage = 1280+50;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[3].voltage = 1360+50;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesLoDisch[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[0].voltage = 1000+100;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[1].voltage = 1200+100;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[1].capacity =  10;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[2].voltage = 1280+100;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[3].voltage = 1360+100;  StoreEE.BattParams[BC_LI_ION].VtoCapacitiesChrg[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LI_ION].hiRateDichargeMins = 60 / 4; // hiRateDichargeMins: 4C rate of discharge   ;
    StoreEE.BattParams[BC_LI_ION].loRateDichargeMins = 60 * 2; // loRateDichargeMins: 0.5C rate of discharge ;

    // LFP
    StoreEE.BattParams[BC_LFP].batFullVoltage     = 1360;   // Voltage in hundredths (V*100), Bat Full 
    StoreEE.BattParams[BC_LFP].warningVoltage     = 1250;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LFP].shutdownVoltage    = 1240;   // V*100, Bat Empty                        
    StoreEE.BattParams[BC_LFP].isChargingVolts    = 1365;   // V*100, Above this, must be charging     
    StoreEE.BattParams[BC_LFP].isDisChargingVolts = 1200;   // V*100, Below this, must be discharging  
    //StoreEE.BattParams[BC_LFP].numCapacityPointsUsed = NUM_CAPACITY_POINTS;   // # of points in voltage/capacity curve
    StoreEE.BattParams[BC_LFP].iCalcdTimeToEmpty  = StoreEE.iAvgTimeToEmpty;   // Runtime calculated time to empty from full
    StoreEE.BattParams[BC_LFP].timeToEmptyCalcState = 0;   // 0:No calc done yet, 1:Partially done, 2:Pretty confident
    // Voltages to Capacities mapping: LFP
    StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[0].voltage = 1000;  StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[1].voltage = 1200;  StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[1].capacity =   9;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[2].voltage = 1290;  StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[3].voltage = 1350;  StoreEE.BattParams[BC_LFP].VtoCapacitiesHiDisch[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[0].voltage = 1000+50;  StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[1].voltage = 1200+50;  StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[1].capacity =   9;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[2].voltage = 1290+50;  StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[3].voltage = 1350+50;  StoreEE.BattParams[BC_LFP].VtoCapacitiesLoDisch[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[0].voltage = 1000+100;  StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[0].capacity =   0;   // Use more resolution at low end of curve
    StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[1].voltage = 1200+100;  StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[1].capacity =   9;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[2].voltage = 1290+100;  StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[2].capacity =  20;   // 
    StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[3].voltage = 1350+100;  StoreEE.BattParams[BC_LFP].VtoCapacitiesChrg[3].capacity = 100;   // Actual: 99%=13.4, 100%=13.6

    StoreEE.BattParams[BC_LFP].hiRateDichargeMins = 60 / 4; // hiRateDichargeMins: 4C rate of discharge   ;
    StoreEE.BattParams[BC_LFP].loRateDichargeMins = 60 * 2; // loRateDichargeMins: 0.5C rate of discharge ;

    memcpy(&StoreEE.BattParams[BC_Other], &StoreEE.BattParams[BC_AGM], sizeof(StoreEE.BattParams[BC_AGM]));   // Init "Other" too


    StoreEE.calibPointLow.voltage   = 0;
    StoreEE.calibPointLow.a2dValue  = 0;
    StoreEE.calibPointHigh.voltage  = 1400;
    StoreEE.calibPointHigh.a2dValue = 1023;  // This will definetely need to be calibrated!
    StoreEE.dischRateMinutes        = 60;   // 1 hour

    // Calculate single v to capacity curve by interpolating between hi and low discharge curves for present battery chemistry
    calcUserCapacityCurve();    

    // Parameters for ACPI compliancy
    StoreEE.iWarnCapacityLimit = 10; // warning at 10%
    StoreEE.iRemnCapacityLimit = 5; // low at 5%
    StoreEE.msgPcEnabledCfgMode = false;
    StoreEE.msgPcEnabledRunMode = true;

    enableDebugPrints(StoreEE.debugFlags);


    Serial1.println(F("Restored factory defaults."));       // DOYET PUT BACK
}



// Calculate single v to capacity curve by interpolating between hi and low discharge curves for present battery chemistry
void calcUserCapacityCurve(void)
{
    uint8_t chem = StoreEE.BatChem;
    uint16_t minsHi = StoreEE.BattParams[chem].hiRateDichargeMins;   // High disch rate, lower voltage
    uint16_t minsLo = StoreEE.BattParams[chem].loRateDichargeMins;   // Low disch rate, higher voltage
    uint16_t minsUser = StoreEE.dischRateMinutes;                    // User specified discharge rate in minutes

    //StoreEE.iAvgTimeToEmpty = StoreEE.dischRateMinutes * 60;
    if (StoreEE.dischRateMinutes > (18*60))
    {
        StoreEE.iAvgTimeToEmpty = ((uint16_t)18 * (uint16_t)60 * (uint16_t)60); // Show reasonable max: 18 hours in seconds
    }
    else
    {
        // Report actual % remaining in seconds
        StoreEE.iAvgTimeToEmpty = StoreEE.dischRateMinutes * 60;
    }


    for (uint8_t i = 0; i < NUM_CAPACITY_POINTS; i++)
    {
        StoreEE.VtoCapacitiesUser[i].voltage = map(minsUser, 
                                                 minsHi, minsLo, 
                                                 StoreEE.BattParams[StoreEE.BatChem].VtoCapacitiesHiDisch[i].voltage, 
                                                 StoreEE.BattParams[StoreEE.BatChem].VtoCapacitiesLoDisch[i].voltage
                                                 );
        StoreEE.VtoCapacitiesUser[i].capacity = map(minsUser,
                                                  minsHi, minsLo, 
                                                  StoreEE.BattParams[StoreEE.BatChem].VtoCapacitiesHiDisch[i].capacity,
                                                  StoreEE.BattParams[StoreEE.BatChem].VtoCapacitiesLoDisch[i].capacity
                                                  );
    }
 }

void enableDebugPrints(uint8_t debugPrBits)
{
    doDebugPrints        = bitRead(debugPrBits, DBG_PRINT_MAIN     ) ? true : false;   // 0    // 0x01 Turn off debug printing for this file
    doVoltageGraphOutput = bitRead(debugPrBits, DBG_PRINT_CSV_VOLTS) ? true : false;   // 0    // 0x02 CSV output
}

