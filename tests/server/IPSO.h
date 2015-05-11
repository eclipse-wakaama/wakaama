/*******************************************************************************
 *
 * Copyright (c) 2015 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation
 *
 *******************************************************************************/


#ifndef _IPSO_H_
#define _IPSO_H_

#define IPSO_Digital_Input_OBJ_ID       3200
#define IPSO_Digital_Output_OBJ_ID      3201
#define IPSO_Analogue_Input_OBJ_ID      3202
#define IPSO_Analogue_Output_OBJ_ID     3203
#define IPSO_Generic_Sensor_OBJ_ID      3300
#define IPSO_Illuminance_Sensor_OBJ_ID  3301
#define IPSO_Presence_Sensor_OBJ_ID     3302
#define IPSO_Temperature_Sensor_OBJ_ID  3303
#define IPSO_Humidity_Sensor_OBJ_ID     3304
#define IPSO_Power_Measurement_OBJ_ID   3305
#define IPSO_Actuation_OBJ_ID           3306
#define IPSO_Set_Point_OBJ_ID           3308
#define IPSO_Load_Control_OBJ_ID        3310
#define IPSO_Light_Control_OBJ_ID       3311
#define IPSO_Power_Control_OBJ_ID       3312
#define IPSO_Accelerometer_OBJ_ID       3313
#define IPSO_Magnetometer_OBJ_ID        3314
#define IPSO_Barometer_OBJ_ID           3315

#define IPSO_Digital_Input_State_RES_ID             5500
#define IPSO_Digital_Input_Counter_RES_ID           5501
#define IPSO_Digital_Input_Polarity_RES_ID          5502
#define IPSO_Digital_Input_Debounce_Period_RES_ID   5503
#define IPSO_Digital_Input_Edge_Selection_RES_ID    5504
#define IPSO_Digital_Input_Counter_Reset_RES_ID     5505

#define IPSO_Digital_Output_State_RES_ID    5550
#define IPSO_Digital_Output_Polarity_RES_ID 5501

#define IPSO_Analog_Input_Current_Value_RES_ID          5600
#define IPSO_Min_Measured_Value_RES_ID                  5601
#define IPSO_Max_Measured_Value_RES_ID                  5602
#define IPSO_Min_Range_Value_RES_ID                     5603
#define IPSO_Max_Range_Value_RES_ID                     5604
#define IPSO_Reset_Min_and_Max_Measured_Values_RES_ID   5605

#define IPSO_Analog_Output_Current_Value_RES_ID 5650

#define IPSO_Sensor_Value_RES_ID        5700
#define IPSO_Sensor_Units_RES_ID        5701
#define IPSO_X_Value_RES_ID             5702
#define IPSO_Y_Value_RES_ID             5703
#define IPSO_Z_Value_RES_ID             5704
#define IPSO_Compass_Direction_RES_ID   5705
#define IPSO_Colour_RES_ID              5706

#define IPSO_Application_Type_RES_ID    5750
#define IPSO_Sensor_Type_RES_ID         5751

#define IPSO_Instantaneous_active_power_RES_ID  5800
#define IPSO_Min_Measured_active_power_RES_ID   5801
#define IPSO_Max_Measured_active_power_RES_ID   5802
#define IPSO_Min_Range_active_power_RES_ID      5803
#define IPSO_Max_Range_active_power_RES_ID      5804
#define IPSO_Cumulative_active_power_RES_ID     5805
#define IPSO_Active_Power_Calibration_RES_ID    5806

#define IPSO_Instantaneous_reactive_power_RES_ID    5810
#define IPSO_Min_Measured_reactive_power_RES_ID     5811
#define IPSO_Max_Measured_reactive_power_RES_ID     5812
#define IPSO_Min_Range_reactive_power_RES_ID        5813
#define IPSO_Max_Range_reactive_power_RES_ID        5814
#define IPSO_Cumulative_reactive_power_RES_ID       5815
#define IPSO_Reactive_Power_Calibration_RES_ID      5816

#define IPSO_Power_factor_RES_ID            5820
#define IPSO_Current_Calibration_RES_ID     5821
#define IPSO_Reset_Cumulative_energy_RES_ID 5822
#define IPSO_Event_Identifier_RES_ID        5823
#define IPSO_Start_Time_RES_ID              5824
#define IPSO_Duration_In_Min_RES_ID         5825
#define IPSO_Criticality_Level_RES_ID       5826
#define IPSO_Avg_Load_AdjPct_RES_ID         5827
#define IPSO_Duty_Cycle_RES_ID              5828

#define IPSO_On_Off_RES_ID                  5850
#define IPSO_Dimmer_RES_ID                  5851
#define IPSO_On_time_RES_ID                 5852
#define IPSO_Muti_state_Output_RES_ID       5853

#define IPSO_SetPoint_Value_RES_ID      5900
#define IPSO_Busy_to_Clear_delay_RES_ID 5903
#define IPSO_Clear_to_Busy_delay_RES_ID 5904


#endif /* IPSO_H_ */
