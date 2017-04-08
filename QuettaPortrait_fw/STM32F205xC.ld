/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * STM32F207xG memory setup.
 * Note: Use of ram1 and ram2 is mutually exclusive with use of ram0.
 */
 
  /* Put it here instead of Makefile */
__main_stack_size__     = 0x0400; 
__process_stack_size__  = 0x0400;
 
MEMORY
{
    flash : org = 0x08000000, len = 256k
    ram0  : org = 0x20000000, len = 96k    /* SRAM1 + SRAM2 */
    ram1  : org = 0x20000000, len = 80k    /* SRAM1 */
    ram2  : org = 0x2001C000, len = 16k     /* SRAM2 */
    ram3  : org = 0x00000000, len = 0
    ram4  : org = 0x10000000, len = 64k     /* CCM SRAM */
    ram5  : org = 0x40024000, len = 4k      /* BCKP SRAM */
    ram6  : org = 0x00000000, len = 0
    ram7  : org = 0x00000000, len = 0
}

/* RAM region to be used for Main stack. This stack accommodates the processing
   of all exceptions and interrupts*/
REGION_ALIAS("MAIN_STACK_RAM", ram0);

/* RAM region to be used for the process stack. This is the stack used by
   the main() function.*/
REGION_ALIAS("PROCESS_STACK_RAM", ram0);

/* RAM region to be used for data segment.*/
REGION_ALIAS("DATA_RAM", ram0);

/* RAM region to be used for BSS segment.*/
REGION_ALIAS("BSS_RAM", ram0);

/* RAM region to be used for the default heap.*/
REGION_ALIAS("HEAP_RAM", ram0);

INCLUDE ../rules.ld
