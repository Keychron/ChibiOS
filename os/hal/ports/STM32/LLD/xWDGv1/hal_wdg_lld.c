/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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

/**
 * @file    xWDGv1/hal_wdg_lld.c
 * @brief   WDG Driver subsystem low level driver source.
 *
 * @addtogroup WDG
 * @{
 */

#include "hal.h"

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

#define KR_KEY_RELOAD                       0xAAAAU
#define KR_KEY_ENABLE                       0xCCCCU
#define KR_KEY_WRITE                        0x5555U
#define KR_KEY_PROTECT                      0x0000U

#if !defined(IWDG) && defined(IWDG1)
#define IWDG                                IWDG1
#endif

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if STM32_WDG_USE_IWDG || defined(__DOXYGEN__)
WDGDriver WDGD1; /* IWDG */
#endif
#if STM32_WDG_USE_WWDG || defined(__DOXYGEN__)
WDGDriver WDGD2; /* WWDG */
#endif

/*===========================================================================*/
/* Driver local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/
/**
 * @brief   Clock initialization.
 *
 * @notapi
 */
#if STM32_WDG_USE_WWDG
static inline void wwdg_enable_clock(void) {
#if defined(RCC_APB1ENR_WWDGEN)
  /* F1/F2/F4 style */
  rccEnableAPB1(RCC_APB1ENR_WWDGEN, true);
#elif defined(RCC_APB1ENR1_WWDGEN)
  /* L4/G4/H7 style (APB1R1) */
  rccEnableAPB1R1(RCC_APB1ENR1_WWDGEN, true);
#elif defined(RCC_APB3ENR_WWDGEN)
  /* Some newer families */
  rccEnableAPB3(RCC_APB3ENR_WWDGEN, true);
#endif
}

static inline void wwdg_disable_clock(void) {
#if defined(RCC_APB1ENR_WWDGEN)
  /* F1/F2/F4 style */
  rccDisableAPB1(RCC_APB1ENR_WWDGEN);
#elif defined(RCC_APB1ENR1_WWDGEN)
  /* L4/G4/H7 style (APB1R1) */
  rccDisableAPB1R1(RCC_APB1ENR1_WWDGEN);
#elif defined(RCC_APB3ENR_WWDGEN)
  /* Some newer families */
  rccDisableAPB1(RCC_APB3ENR_WWDGEN);
#endif
}
#endif /* STM32_WDG_USE_WWDG */

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Low level WDG driver initialization.
 *
 * @notapi
 */
void wdg_lld_init(void) {

#if STM32_WDG_USE_IWDG
  WDGD1.state = WDG_STOP;
  WDGD1.type  = WDG_PERIPH_IWDG;
  WDGD1.reg.iwdg = IWDG;
#endif

#if STM32_WDG_USE_WWDG
  WDGD2.state = WDG_STOP;
  WDGD2.type  = WDG_PERIPH_WWDG;
  WDGD2.reg.wwdg = WWDG;
#endif
}

/**
 * @brief   Configures and activates the WDG peripheral.
 * @param[in] wdgp      pointer to the @p WDGDriver object
 * @notapi
 */
void wdg_lld_start(WDGDriver *wdgp) {
#if STM32_WDG_USE_IWDG || STM32_WDG_USE_WWDG
  if (wdgp->type == WDG_PERIPH_IWDG) {
#if STM32_WDG_USE_IWDG
    /* Enable IWDG and unlock for write. */
    wdgp->reg.iwdg->KR   = KR_KEY_ENABLE;
    wdgp->reg.iwdg->KR   = KR_KEY_WRITE;

    /* Write configuration. */
    wdgp->reg.iwdg->PR   = wdgp->config->pr;
    wdgp->reg.iwdg->RLR  = wdgp->config->rlr;

    /* Wait the registers to be updated. */
    while (wdgp->wdg->SR != 0)
      ;

  #if STM32_IWDG_IS_WINDOWED
    /* This also triggers a refresh. */
    wdgp->reg.iwdg->WINR = wdgp->config->winr;
  #else
    wdgp->reg.iwdg->KR   = KR_KEY_RELOAD;
  #endif
#endif /* STM32_WDG_USE_IWDG */
  }
  else { /* WWDG */
#if STM32_WDG_USE_WWDG
    /* Ensure peripheral clock. */
    wwdg_enable_clock();

    /* Program CFR (prescaler + window) and initial counter (T) with WDGA set. */
    wdgp->reg.wwdg->CFR = wdgp->config->cfr;

    /* Ensure T fits mask; keep WDGA set. */
    uint32_t t = (uint32_t)(wdgp->config->cr) & (WWDG_CR_T);
    wdgp->reg.wwdg->CR = (uint32_t)WWDG_CR_WDGA | t;
#endif /* STM32_WDG_USE_WWDG */
  }
#endif /* any */
}
#include "hal.h"
/**
 * @brief   Deactivates the WDG peripheral.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_stop(WDGDriver *wdgp) {

  if (wdgp->type == WDG_PERIPH_IWDG) {
#if STM32_WDG_USE_IWDG
    /* IWDG cannot be stopped once activated. */
    osalDbgAssert(wdgp->state == WDG_STOP,
                  "IWDG cannot be stopped once activated");
#endif
  }
  else {
#if STM32_WDG_USE_WWDG
    /* Disarm WWDG by clearing WDGA (write a valid T, WDGA=0).
     * NOTE: writing 0 for CR can put the hardware counter into an
     * invalid/low value and on some STM32 families this immediately
     * triggers a reset. Use the same T value used at start but clear
     * the WDGA bit to disable the watchdog without forcing an
     * out-of-range counter write.
     */
    {
      wdgp->reg.wwdg->CR = WWDG_CR_T; /* WDGA cleared */
      wwdg_disable_clock();
    }
#endif
  }
}

/**
 * @brief   Reloads WDG's counter.
 *
 * @param[in] wdgp      pointer to the @p WDGDriver object
 *
 * @notapi
 */
void wdg_lld_reset(WDGDriver *wdgp) {
  if (wdgp->type == WDG_PERIPH_IWDG) {
#if STM32_WDG_USE_IWDG
    wdgp->reg.iwdg->KR = KR_KEY_RELOAD;
#endif
  }
  else {
#if STM32_WDG_USE_WWDG
    /* Refresh by rewriting T (config->cr), keep WDGA set. */
    uint32_t t = ((uint32_t)wdgp->config->cr) & WWDG_CR_T;
    uint32_t cr = wdgp->reg.wwdg->CR;
    cr &= ~(uint32_t)WWDG_CR_T;
    wdgp->reg.wwdg->CR = cr | t | (uint32_t)WWDG_CR_WDGA;
#endif
  }
}

#endif /* HAL_USE_WDG == TRUE */
