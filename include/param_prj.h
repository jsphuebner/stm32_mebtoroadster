/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

 //Define a version string of your firmware here
#define VER 1.00.R

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 3
//Next value Id: 2375
/*              category     name         unit       min     max     default id */
#define BMB_SHEET_VALUE_ENTRIES(sheet, base) \
    VALUE_ENTRY(bmb##sheet##_bal_min_v, "raw",     base + 0 ) \
    VALUE_ENTRY(bmb##sheet##_bal_min_brick, "id",  base + 1 ) \
    VALUE_ENTRY(bmb##sheet##_bal_max_v, "raw",     base + 2 ) \
    VALUE_ENTRY(bmb##sheet##_bal_max_brick, "id",  base + 3 ) \
    VALUE_ENTRY(bmb##sheet##_v_min, "raw",         base + 4 ) \
    VALUE_ENTRY(bmb##sheet##_v_max, "raw",         base + 5 ) \
    VALUE_ENTRY(bmb##sheet##_v_sum_avg, "raw",     base + 6 ) \
    VALUE_ENTRY(bmb##sheet##_v_min_brick, "id",    base + 7 ) \
    VALUE_ENTRY(bmb##sheet##_v_max_brick, "id",    base + 8 ) \
    VALUE_ENTRY(bmb##sheet##_t_min, "raw",         base + 9 ) \
    VALUE_ENTRY(bmb##sheet##_t_max, "raw",         base + 10) \
    VALUE_ENTRY(bmb##sheet##_t_avg, "raw",         base + 11) \
    VALUE_ENTRY(bmb##sheet##_t_min_therm, "id",    base + 12) \
    VALUE_ENTRY(bmb##sheet##_t_max_therm, "id",    base + 13) \
    VALUE_ENTRY(bmb##sheet##_bleed, "bitmask",     base + 14) \
    VALUE_ENTRY(bmb##sheet##_daisy_rx, "bit",      base + 15) \
    VALUE_ENTRY(bmb##sheet##_over_v, "bit",        base + 16) \
    VALUE_ENTRY(bmb##sheet##_cell_reversal, "bit", base + 17) \
    VALUE_ENTRY(bmb##sheet##_can_pwr_ok, "bit",    base + 18) \
    VALUE_ENTRY(bmb##sheet##_sheet_alarm, "bit",   base + 19) \
    VALUE_ENTRY(bmb##sheet##_pic_pgm, "bit",       base + 20) \
    VALUE_ENTRY(bmb##sheet##_pic_pgc, "bit",       base + 21) \
    VALUE_ENTRY(bmb##sheet##_pic_pgd, "bit",       base + 22) \
    VALUE_ENTRY(bmb##sheet##_alarm_reason, "id",   base + 23) \
    VALUE_ENTRY(bmb##sheet##_alarm_brick, "id",    base + 24)

#define PARAM_LIST \
    PARAM_ENTRY(CAT_COMM,    canspeed,    CANSPEEDS, 0,      4,      2,      1   ) \
    PARAM_ENTRY(CAT_COMM,    canperiod,   CANPERIODS,0,      1,      0,      2   ) \
    PARAM_ENTRY(CAT_TEST,    testparam,   "Hz",      -100,   1000,   0,      0   ) \
    VALUE_ENTRY(opmode,      OPMODES, 2000 ) \
    VALUE_ENTRY(version,     VERSTR,  2001 ) \
    VALUE_ENTRY(lasterr,     errorListString,  2002 ) \
    VALUE_ENTRY(testain,     "dig",   2003 ) \
    VALUE_ENTRY(cpuload,     "%",     2004 ) \
    BMB_SHEET_VALUE_ENTRIES(1, 2100) \
    BMB_SHEET_VALUE_ENTRIES(2, 2125) \
    BMB_SHEET_VALUE_ENTRIES(3, 2150) \
    BMB_SHEET_VALUE_ENTRIES(4, 2175) \
    BMB_SHEET_VALUE_ENTRIES(5, 2200) \
    BMB_SHEET_VALUE_ENTRIES(6, 2225) \
    BMB_SHEET_VALUE_ENTRIES(7, 2250) \
    BMB_SHEET_VALUE_ENTRIES(8, 2275) \
    BMB_SHEET_VALUE_ENTRIES(9, 2300) \
    BMB_SHEET_VALUE_ENTRIES(10, 2325) \
    BMB_SHEET_VALUE_ENTRIES(11, 2350)


/***** Enum String definitions *****/
#define OPMODES      "0=Off, 1=Run"
#define CANSPEEDS    "0=125k, 1=250k, 2=500k, 3=800k, 4=1M"
#define CANPERIODS   "0=100ms, 1=10ms"
#define CAT_TEST     "Testing"
#define CAT_COMM     "Communication"

#define VERSTR STRINGIFY(4=VER-name)

/***** enums ******/


enum _canperiods
{
   CAN_PERIOD_100MS = 0,
   CAN_PERIOD_10MS,
   CAN_PERIOD_LAST
};

enum _modes
{
   MOD_OFF = 0,
   MOD_RUN,
   MOD_LAST
};

//Generated enum-string for possible errors
extern const char* errorListString;
