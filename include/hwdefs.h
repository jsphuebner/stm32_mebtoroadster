#ifndef HWDEFS_H_INCLUDED
#define HWDEFS_H_INCLUDED


//Common for any config

#define RCC_CLOCK_SETUP() rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ])
#define OVER_CUR_TIMER     TIM4
#define OCURMAX            4096

//Address of parameter/CAN map blocks in flash
#define FLASH_PAGE_SIZE 2048
#define PARAM_BLKSIZE FLASH_PAGE_SIZE
#define PARAM_BLKNUM  1   //last block of 2k
#define CAN1_BLKNUM   3   //used as: flash_end - (CAN1_BLKNUM * FLASH_PAGE_SIZE) => 3rd 2k block from flash end
#define CAN2_BLKNUM   4   //used as: flash_end - (CAN2_BLKNUM * FLASH_PAGE_SIZE) => 4th 2k block from flash end


#endif // HWDEFS_H_INCLUDED
