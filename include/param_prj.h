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
#define VER 1.01.R

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 9
//Next value Id: 2389
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_COMM,    canspeed,    CANSPEEDS, 0,      4,      2,      1   ) \
    PARAM_ENTRY(CAT_COMM,    canperiod,   CANPERIODS,0,      1,      0,      2   ) \
    PARAM_ENTRY(CAT_COMM,    cdmcurlim,   "A",       0,      255,    255,    3   ) \
    PARAM_ENTRY(CAT_BMS,     cellmin,     "mV",      2500,   3600,   3380,   4   ) \
    PARAM_ENTRY(CAT_BMS,     cellmax,     "mV",      4000,   4300,   4200,   5   ) \
    PARAM_ENTRY(CAT_BMS,     ahmax,       "Ah",      10,     200,    148,    6   ) \
    PARAM_ENTRY(CAT_BMS,     chargekp,    "",        0,      200,    3,      7   ) \
    PARAM_ENTRY(CAT_BMS,     chargeki,    "",        0,      200,    3,      8   ) \
    PARAM_ENTRY(CAT_TEST,    testparam,   "Hz",      -100,   1000,   0,      0   ) \
    VALUE_ENTRY(opmode,      OPMODES, 2000 ) \
    VALUE_ENTRY(version,     VERSTR,  2001 ) \
    VALUE_ENTRY(lasterr,     errorListString,  2002 ) \
    VALUE_ENTRY(cpuload,     "%",     2004 ) \
    VALUE_ENTRY(bmb1_bal_min_v, "raw", 2100) \
    VALUE_ENTRY(bmb1_bal_min_brick, "id", 2101) \
    VALUE_ENTRY(bmb1_bal_max_v, "raw", 2102) \
    VALUE_ENTRY(bmb1_bal_max_brick, "id", 2103) \
    VALUE_ENTRY(bmb1_v_min, "raw", 2104) \
    VALUE_ENTRY(bmb1_v_max, "raw", 2105) \
    VALUE_ENTRY(bmb1_v_sum_avg, "raw", 2106) \
    VALUE_ENTRY(bmb1_v_min_brick, "id", 2107) \
    VALUE_ENTRY(bmb1_v_max_brick, "id", 2108) \
    VALUE_ENTRY(bmb1_t_min, "raw", 2109) \
    VALUE_ENTRY(bmb1_t_max, "raw", 2110) \
    VALUE_ENTRY(bmb1_t_avg, "raw", 2111) \
    VALUE_ENTRY(bmb1_t_min_therm, "id", 2112) \
    VALUE_ENTRY(bmb1_t_max_therm, "id", 2113) \
    VALUE_ENTRY(bmb1_bleed, "bitmask", 2114) \
    VALUE_ENTRY(bmb1_daisy_rx, "bit", 2115) \
    VALUE_ENTRY(bmb1_over_v, "bit", 2116) \
    VALUE_ENTRY(bmb1_cell_reversal, "bit", 2117) \
    VALUE_ENTRY(bmb1_can_pwr_ok, "bit", 2118) \
    VALUE_ENTRY(bmb1_sheet_alarm, "bit", 2119) \
    VALUE_ENTRY(bmb1_pic_pgm, "bit", 2120) \
    VALUE_ENTRY(bmb1_pic_pgc, "bit", 2121) \
    VALUE_ENTRY(bmb1_pic_pgd, "bit", 2122) \
    VALUE_ENTRY(bmb1_alarm_reason, "id", 2123) \
    VALUE_ENTRY(bmb1_alarm_brick, "id", 2124) \
    VALUE_ENTRY(bmb2_bal_min_v, "raw", 2125) \
    VALUE_ENTRY(bmb2_bal_min_brick, "id", 2126) \
    VALUE_ENTRY(bmb2_bal_max_v, "raw", 2127) \
    VALUE_ENTRY(bmb2_bal_max_brick, "id", 2128) \
    VALUE_ENTRY(bmb2_v_min, "raw", 2129) \
    VALUE_ENTRY(bmb2_v_max, "raw", 2130) \
    VALUE_ENTRY(bmb2_v_sum_avg, "raw", 2131) \
    VALUE_ENTRY(bmb2_v_min_brick, "id", 2132) \
    VALUE_ENTRY(bmb2_v_max_brick, "id", 2133) \
    VALUE_ENTRY(bmb2_t_min, "raw", 2134) \
    VALUE_ENTRY(bmb2_t_max, "raw", 2135) \
    VALUE_ENTRY(bmb2_t_avg, "raw", 2136) \
    VALUE_ENTRY(bmb2_t_min_therm, "id", 2137) \
    VALUE_ENTRY(bmb2_t_max_therm, "id", 2138) \
    VALUE_ENTRY(bmb2_bleed, "bitmask", 2139) \
    VALUE_ENTRY(bmb2_daisy_rx, "bit", 2140) \
    VALUE_ENTRY(bmb2_over_v, "bit", 2141) \
    VALUE_ENTRY(bmb2_cell_reversal, "bit", 2142) \
    VALUE_ENTRY(bmb2_can_pwr_ok, "bit", 2143) \
    VALUE_ENTRY(bmb2_sheet_alarm, "bit", 2144) \
    VALUE_ENTRY(bmb2_pic_pgm, "bit", 2145) \
    VALUE_ENTRY(bmb2_pic_pgc, "bit", 2146) \
    VALUE_ENTRY(bmb2_pic_pgd, "bit", 2147) \
    VALUE_ENTRY(bmb2_alarm_reason, "id", 2148) \
    VALUE_ENTRY(bmb2_alarm_brick, "id", 2149) \
    VALUE_ENTRY(bmb3_bal_min_v, "raw", 2150) \
    VALUE_ENTRY(bmb3_bal_min_brick, "id", 2151) \
    VALUE_ENTRY(bmb3_bal_max_v, "raw", 2152) \
    VALUE_ENTRY(bmb3_bal_max_brick, "id", 2153) \
    VALUE_ENTRY(bmb3_v_min, "raw", 2154) \
    VALUE_ENTRY(bmb3_v_max, "raw", 2155) \
    VALUE_ENTRY(bmb3_v_sum_avg, "raw", 2156) \
    VALUE_ENTRY(bmb3_v_min_brick, "id", 2157) \
    VALUE_ENTRY(bmb3_v_max_brick, "id", 2158) \
    VALUE_ENTRY(bmb3_t_min, "raw", 2159) \
    VALUE_ENTRY(bmb3_t_max, "raw", 2160) \
    VALUE_ENTRY(bmb3_t_avg, "raw", 2161) \
    VALUE_ENTRY(bmb3_t_min_therm, "id", 2162) \
    VALUE_ENTRY(bmb3_t_max_therm, "id", 2163) \
    VALUE_ENTRY(bmb3_bleed, "bitmask", 2164) \
    VALUE_ENTRY(bmb3_daisy_rx, "bit", 2165) \
    VALUE_ENTRY(bmb3_over_v, "bit", 2166) \
    VALUE_ENTRY(bmb3_cell_reversal, "bit", 2167) \
    VALUE_ENTRY(bmb3_can_pwr_ok, "bit", 2168) \
    VALUE_ENTRY(bmb3_sheet_alarm, "bit", 2169) \
    VALUE_ENTRY(bmb3_pic_pgm, "bit", 2170) \
    VALUE_ENTRY(bmb3_pic_pgc, "bit", 2171) \
    VALUE_ENTRY(bmb3_pic_pgd, "bit", 2172) \
    VALUE_ENTRY(bmb3_alarm_reason, "id", 2173) \
    VALUE_ENTRY(bmb3_alarm_brick, "id", 2174) \
    VALUE_ENTRY(bmb4_bal_min_v, "raw", 2175) \
    VALUE_ENTRY(bmb4_bal_min_brick, "id", 2176) \
    VALUE_ENTRY(bmb4_bal_max_v, "raw", 2177) \
    VALUE_ENTRY(bmb4_bal_max_brick, "id", 2178) \
    VALUE_ENTRY(bmb4_v_min, "raw", 2179) \
    VALUE_ENTRY(bmb4_v_max, "raw", 2180) \
    VALUE_ENTRY(bmb4_v_sum_avg, "raw", 2181) \
    VALUE_ENTRY(bmb4_v_min_brick, "id", 2182) \
    VALUE_ENTRY(bmb4_v_max_brick, "id", 2183) \
    VALUE_ENTRY(bmb4_t_min, "raw", 2184) \
    VALUE_ENTRY(bmb4_t_max, "raw", 2185) \
    VALUE_ENTRY(bmb4_t_avg, "raw", 2186) \
    VALUE_ENTRY(bmb4_t_min_therm, "id", 2187) \
    VALUE_ENTRY(bmb4_t_max_therm, "id", 2188) \
    VALUE_ENTRY(bmb4_bleed, "bitmask", 2189) \
    VALUE_ENTRY(bmb4_daisy_rx, "bit", 2190) \
    VALUE_ENTRY(bmb4_over_v, "bit", 2191) \
    VALUE_ENTRY(bmb4_cell_reversal, "bit", 2192) \
    VALUE_ENTRY(bmb4_can_pwr_ok, "bit", 2193) \
    VALUE_ENTRY(bmb4_sheet_alarm, "bit", 2194) \
    VALUE_ENTRY(bmb4_pic_pgm, "bit", 2195) \
    VALUE_ENTRY(bmb4_pic_pgc, "bit", 2196) \
    VALUE_ENTRY(bmb4_pic_pgd, "bit", 2197) \
    VALUE_ENTRY(bmb4_alarm_reason, "id", 2198) \
    VALUE_ENTRY(bmb4_alarm_brick, "id", 2199) \
    VALUE_ENTRY(bmb5_bal_min_v, "raw", 2200) \
    VALUE_ENTRY(bmb5_bal_min_brick, "id", 2201) \
    VALUE_ENTRY(bmb5_bal_max_v, "raw", 2202) \
    VALUE_ENTRY(bmb5_bal_max_brick, "id", 2203) \
    VALUE_ENTRY(bmb5_v_min, "raw", 2204) \
    VALUE_ENTRY(bmb5_v_max, "raw", 2205) \
    VALUE_ENTRY(bmb5_v_sum_avg, "raw", 2206) \
    VALUE_ENTRY(bmb5_v_min_brick, "id", 2207) \
    VALUE_ENTRY(bmb5_v_max_brick, "id", 2208) \
    VALUE_ENTRY(bmb5_t_min, "raw", 2209) \
    VALUE_ENTRY(bmb5_t_max, "raw", 2210) \
    VALUE_ENTRY(bmb5_t_avg, "raw", 2211) \
    VALUE_ENTRY(bmb5_t_min_therm, "id", 2212) \
    VALUE_ENTRY(bmb5_t_max_therm, "id", 2213) \
    VALUE_ENTRY(bmb5_bleed, "bitmask", 2214) \
    VALUE_ENTRY(bmb5_daisy_rx, "bit", 2215) \
    VALUE_ENTRY(bmb5_over_v, "bit", 2216) \
    VALUE_ENTRY(bmb5_cell_reversal, "bit", 2217) \
    VALUE_ENTRY(bmb5_can_pwr_ok, "bit", 2218) \
    VALUE_ENTRY(bmb5_sheet_alarm, "bit", 2219) \
    VALUE_ENTRY(bmb5_pic_pgm, "bit", 2220) \
    VALUE_ENTRY(bmb5_pic_pgc, "bit", 2221) \
    VALUE_ENTRY(bmb5_pic_pgd, "bit", 2222) \
    VALUE_ENTRY(bmb5_alarm_reason, "id", 2223) \
    VALUE_ENTRY(bmb5_alarm_brick, "id", 2224) \
    VALUE_ENTRY(bmb6_bal_min_v, "raw", 2225) \
    VALUE_ENTRY(bmb6_bal_min_brick, "id", 2226) \
    VALUE_ENTRY(bmb6_bal_max_v, "raw", 2227) \
    VALUE_ENTRY(bmb6_bal_max_brick, "id", 2228) \
    VALUE_ENTRY(bmb6_v_min, "raw", 2229) \
    VALUE_ENTRY(bmb6_v_max, "raw", 2230) \
    VALUE_ENTRY(bmb6_v_sum_avg, "raw", 2231) \
    VALUE_ENTRY(bmb6_v_min_brick, "id", 2232) \
    VALUE_ENTRY(bmb6_v_max_brick, "id", 2233) \
    VALUE_ENTRY(bmb6_t_min, "raw", 2234) \
    VALUE_ENTRY(bmb6_t_max, "raw", 2235) \
    VALUE_ENTRY(bmb6_t_avg, "raw", 2236) \
    VALUE_ENTRY(bmb6_t_min_therm, "id", 2237) \
    VALUE_ENTRY(bmb6_t_max_therm, "id", 2238) \
    VALUE_ENTRY(bmb6_bleed, "bitmask", 2239) \
    VALUE_ENTRY(bmb6_daisy_rx, "bit", 2240) \
    VALUE_ENTRY(bmb6_over_v, "bit", 2241) \
    VALUE_ENTRY(bmb6_cell_reversal, "bit", 2242) \
    VALUE_ENTRY(bmb6_can_pwr_ok, "bit", 2243) \
    VALUE_ENTRY(bmb6_sheet_alarm, "bit", 2244) \
    VALUE_ENTRY(bmb6_pic_pgm, "bit", 2245) \
    VALUE_ENTRY(bmb6_pic_pgc, "bit", 2246) \
    VALUE_ENTRY(bmb6_pic_pgd, "bit", 2247) \
    VALUE_ENTRY(bmb6_alarm_reason, "id", 2248) \
    VALUE_ENTRY(bmb6_alarm_brick, "id", 2249) \
    VALUE_ENTRY(bmb7_bal_min_v, "raw", 2250) \
    VALUE_ENTRY(bmb7_bal_min_brick, "id", 2251) \
    VALUE_ENTRY(bmb7_bal_max_v, "raw", 2252) \
    VALUE_ENTRY(bmb7_bal_max_brick, "id", 2253) \
    VALUE_ENTRY(bmb7_v_min, "raw", 2254) \
    VALUE_ENTRY(bmb7_v_max, "raw", 2255) \
    VALUE_ENTRY(bmb7_v_sum_avg, "raw", 2256) \
    VALUE_ENTRY(bmb7_v_min_brick, "id", 2257) \
    VALUE_ENTRY(bmb7_v_max_brick, "id", 2258) \
    VALUE_ENTRY(bmb7_t_min, "raw", 2259) \
    VALUE_ENTRY(bmb7_t_max, "raw", 2260) \
    VALUE_ENTRY(bmb7_t_avg, "raw", 2261) \
    VALUE_ENTRY(bmb7_t_min_therm, "id", 2262) \
    VALUE_ENTRY(bmb7_t_max_therm, "id", 2263) \
    VALUE_ENTRY(bmb7_bleed, "bitmask", 2264) \
    VALUE_ENTRY(bmb7_daisy_rx, "bit", 2265) \
    VALUE_ENTRY(bmb7_over_v, "bit", 2266) \
    VALUE_ENTRY(bmb7_cell_reversal, "bit", 2267) \
    VALUE_ENTRY(bmb7_can_pwr_ok, "bit", 2268) \
    VALUE_ENTRY(bmb7_sheet_alarm, "bit", 2269) \
    VALUE_ENTRY(bmb7_pic_pgm, "bit", 2270) \
    VALUE_ENTRY(bmb7_pic_pgc, "bit", 2271) \
    VALUE_ENTRY(bmb7_pic_pgd, "bit", 2272) \
    VALUE_ENTRY(bmb7_alarm_reason, "id", 2273) \
    VALUE_ENTRY(bmb7_alarm_brick, "id", 2274) \
    VALUE_ENTRY(bmb8_bal_min_v, "raw", 2275) \
    VALUE_ENTRY(bmb8_bal_min_brick, "id", 2276) \
    VALUE_ENTRY(bmb8_bal_max_v, "raw", 2277) \
    VALUE_ENTRY(bmb8_bal_max_brick, "id", 2278) \
    VALUE_ENTRY(bmb8_v_min, "raw", 2279) \
    VALUE_ENTRY(bmb8_v_max, "raw", 2280) \
    VALUE_ENTRY(bmb8_v_sum_avg, "raw", 2281) \
    VALUE_ENTRY(bmb8_v_min_brick, "id", 2282) \
    VALUE_ENTRY(bmb8_v_max_brick, "id", 2283) \
    VALUE_ENTRY(bmb8_t_min, "raw", 2284) \
    VALUE_ENTRY(bmb8_t_max, "raw", 2285) \
    VALUE_ENTRY(bmb8_t_avg, "raw", 2286) \
    VALUE_ENTRY(bmb8_t_min_therm, "id", 2287) \
    VALUE_ENTRY(bmb8_t_max_therm, "id", 2288) \
    VALUE_ENTRY(bmb8_bleed, "bitmask", 2289) \
    VALUE_ENTRY(bmb8_daisy_rx, "bit", 2290) \
    VALUE_ENTRY(bmb8_over_v, "bit", 2291) \
    VALUE_ENTRY(bmb8_cell_reversal, "bit", 2292) \
    VALUE_ENTRY(bmb8_can_pwr_ok, "bit", 2293) \
    VALUE_ENTRY(bmb8_sheet_alarm, "bit", 2294) \
    VALUE_ENTRY(bmb8_pic_pgm, "bit", 2295) \
    VALUE_ENTRY(bmb8_pic_pgc, "bit", 2296) \
    VALUE_ENTRY(bmb8_pic_pgd, "bit", 2297) \
    VALUE_ENTRY(bmb8_alarm_reason, "id", 2298) \
    VALUE_ENTRY(bmb8_alarm_brick, "id", 2299) \
    VALUE_ENTRY(bmb9_bal_min_v, "raw", 2300) \
    VALUE_ENTRY(bmb9_bal_min_brick, "id", 2301) \
    VALUE_ENTRY(bmb9_bal_max_v, "raw", 2302) \
    VALUE_ENTRY(bmb9_bal_max_brick, "id", 2303) \
    VALUE_ENTRY(bmb9_v_min, "raw", 2304) \
    VALUE_ENTRY(bmb9_v_max, "raw", 2305) \
    VALUE_ENTRY(bmb9_v_sum_avg, "raw", 2306) \
    VALUE_ENTRY(bmb9_v_min_brick, "id", 2307) \
    VALUE_ENTRY(bmb9_v_max_brick, "id", 2308) \
    VALUE_ENTRY(bmb9_t_min, "raw", 2309) \
    VALUE_ENTRY(bmb9_t_max, "raw", 2310) \
    VALUE_ENTRY(bmb9_t_avg, "raw", 2311) \
    VALUE_ENTRY(bmb9_t_min_therm, "id", 2312) \
    VALUE_ENTRY(bmb9_t_max_therm, "id", 2313) \
    VALUE_ENTRY(bmb9_bleed, "bitmask", 2314) \
    VALUE_ENTRY(bmb9_daisy_rx, "bit", 2315) \
    VALUE_ENTRY(bmb9_over_v, "bit", 2316) \
    VALUE_ENTRY(bmb9_cell_reversal, "bit", 2317) \
    VALUE_ENTRY(bmb9_can_pwr_ok, "bit", 2318) \
    VALUE_ENTRY(bmb9_sheet_alarm, "bit", 2319) \
    VALUE_ENTRY(bmb9_pic_pgm, "bit", 2320) \
    VALUE_ENTRY(bmb9_pic_pgc, "bit", 2321) \
    VALUE_ENTRY(bmb9_pic_pgd, "bit", 2322) \
    VALUE_ENTRY(bmb9_alarm_reason, "id", 2323) \
    VALUE_ENTRY(bmb9_alarm_brick, "id", 2324) \
    VALUE_ENTRY(bmb10_bal_min_v, "raw", 2325) \
    VALUE_ENTRY(bmb10_bal_min_brick, "id", 2326) \
    VALUE_ENTRY(bmb10_bal_max_v, "raw", 2327) \
    VALUE_ENTRY(bmb10_bal_max_brick, "id", 2328) \
    VALUE_ENTRY(bmb10_v_min, "raw", 2329) \
    VALUE_ENTRY(bmb10_v_max, "raw", 2330) \
    VALUE_ENTRY(bmb10_v_sum_avg, "raw", 2331) \
    VALUE_ENTRY(bmb10_v_min_brick, "id", 2332) \
    VALUE_ENTRY(bmb10_v_max_brick, "id", 2333) \
    VALUE_ENTRY(bmb10_t_min, "raw", 2334) \
    VALUE_ENTRY(bmb10_t_max, "raw", 2335) \
    VALUE_ENTRY(bmb10_t_avg, "raw", 2336) \
    VALUE_ENTRY(bmb10_t_min_therm, "id", 2337) \
    VALUE_ENTRY(bmb10_t_max_therm, "id", 2338) \
    VALUE_ENTRY(bmb10_bleed, "bitmask", 2339) \
    VALUE_ENTRY(bmb10_daisy_rx, "bit", 2340) \
    VALUE_ENTRY(bmb10_over_v, "bit", 2341) \
    VALUE_ENTRY(bmb10_cell_reversal, "bit", 2342) \
    VALUE_ENTRY(bmb10_can_pwr_ok, "bit", 2343) \
    VALUE_ENTRY(bmb10_sheet_alarm, "bit", 2344) \
    VALUE_ENTRY(bmb10_pic_pgm, "bit", 2345) \
    VALUE_ENTRY(bmb10_pic_pgc, "bit", 2346) \
    VALUE_ENTRY(bmb10_pic_pgd, "bit", 2347) \
    VALUE_ENTRY(bmb10_alarm_reason, "id", 2348) \
    VALUE_ENTRY(bmb10_alarm_brick, "id", 2349) \
    VALUE_ENTRY(bmb11_bal_min_v, "raw", 2350) \
    VALUE_ENTRY(bmb11_bal_min_brick, "id", 2351) \
    VALUE_ENTRY(bmb11_bal_max_v, "raw", 2352) \
    VALUE_ENTRY(bmb11_bal_max_brick, "id", 2353) \
    VALUE_ENTRY(bmb11_v_min, "raw", 2354) \
    VALUE_ENTRY(bmb11_v_max, "raw", 2355) \
    VALUE_ENTRY(bmb11_v_sum_avg, "raw", 2356) \
    VALUE_ENTRY(bmb11_v_min_brick, "id", 2357) \
    VALUE_ENTRY(bmb11_v_max_brick, "id", 2358) \
    VALUE_ENTRY(bmb11_t_min, "raw", 2359) \
    VALUE_ENTRY(bmb11_t_max, "raw", 2360) \
    VALUE_ENTRY(bmb11_t_avg, "raw", 2361) \
    VALUE_ENTRY(bmb11_t_min_therm, "id", 2362) \
    VALUE_ENTRY(bmb11_t_max_therm, "id", 2363) \
    VALUE_ENTRY(bmb11_bleed, "bitmask", 2364) \
    VALUE_ENTRY(bmb11_daisy_rx, "bit", 2365) \
    VALUE_ENTRY(bmb11_over_v, "bit", 2366) \
    VALUE_ENTRY(bmb11_cell_reversal, "bit", 2367) \
    VALUE_ENTRY(bmb11_can_pwr_ok, "bit", 2368) \
    VALUE_ENTRY(bmb11_sheet_alarm, "bit", 2369) \
    VALUE_ENTRY(bmb11_pic_pgm, "bit", 2370) \
    VALUE_ENTRY(bmb11_pic_pgc, "bit", 2371) \
    VALUE_ENTRY(bmb11_pic_pgd, "bit", 2372) \
    VALUE_ENTRY(bmb11_alarm_reason, "id", 2373) \
    VALUE_ENTRY(bmb11_alarm_brick, "id", 2374) \
    VALUE_ENTRY(cdm_bat_vtg,    "V",   2375) \
    VALUE_ENTRY(cdm_target_vtg, "V",   2376) \
    VALUE_ENTRY(cdm_cur_req,    "A",   2377) \
    VALUE_ENTRY(cdm_soc,        "%",   2378) \
    VALUE_ENTRY(cdm_enabled,    ONOFF, 2379) \
    VALUE_ENTRY(cdm_chg_max_cur, "A",  2385) \
    VALUE_ENTRY(cdm_chg_cur,     "A",  2386) \
    VALUE_ENTRY(cdm_chg_vtg,     "V",  2387) \
    VALUE_ENTRY(cdm_chg_status,  "",   2388)



/***** Enum String definitions *****/
#define OPMODES      "0=Off, 1=Run"
#define ONOFF        "0=Off, 1=On"
#define CANSPEEDS    "0=125k, 1=250k, 2=500k, 3=800k, 4=1M"
#define CANPERIODS   "0=100ms, 1=10ms"
#define CAT_TEST     "Testing"
#define CAT_COMM     "Communication"
#define CAT_BMS      "BMS"

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
