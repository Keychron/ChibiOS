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
 * @file    xWDGv1/hal_wdg_lld.h
 * @brief   WDG Driver subsystem low level driver header.
 *
 * @addtogroup WDG
 * @{
 */

#ifndef HAL_WDG_LLD_H
#define HAL_WDG_LLD_H

#if (HAL_USE_WDG == TRUE) || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/**
 * @name    IWDG RLR register definitions
 * @{
 */
#define STM32_IWDG_RL_MASK                  (0x00000FFF << 0)
#define STM32_IWDG_RL(n)                    ((n) << 0)
/** @} */

/**
 * @name    IWDG PR register definitions
 * @{
 */
#define STM32_IWDG_PR_MASK                  (7 << 0)
#define STM32_IWDG_PR_4                     0U
#define STM32_IWDG_PR_8                     1U
#define STM32_IWDG_PR_16                    2U
#define STM32_IWDG_PR_32                    3U
#define STM32_IWDG_PR_64                    4U
#define STM32_IWDG_PR_128                   5U
#define STM32_IWDG_PR_256                   6U
/** @} */

/**
 * @name    IWDG WINR register definitions (windowed families only)
 * @{
 */
#define STM32_IWDG_WIN_MASK                 (0x00000FFF << 0)
#define STM32_IWDG_WIN(n)                   ((n) << 0)
#define STM32_IWDG_WIN_DISABLED             STM32_IWDG_WIN(0x00000FFF)
/** @} */

/**
 * @name    WWDG CR/ CFR helper masks
 * @note    Use CMSIS device headers for exact bit definitions if available.
 * @{
 */
#if defined(WWDG)
#ifndef WWDG_CR_WDGA
#define WWDG_CR_WDGA                        (1U << 7)
#endif
#ifndef WWDG_CR_T
#define WWDG_CR_T                           (0x7FU << 0)
#endif
#ifndef WWDG_CFR_W
#define WWDG_CFR_W                          (0x7FU << 0)
#endif
#ifndef WWDG_CFR_WDGTB
#define WWDG_CFR_WDGTB                      (3U << 7)
#endif
#endif /* defined(WWDG) */
/** @} */

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/**
 * @name    Configuration options
 * @{
 */
/**
 * @brief   IWDG driver enable switch.
 * @details If set to @p TRUE the support for IWDG is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_WDG_USE_IWDG) || defined(__DOXYGEN__)
#define STM32_WDG_USE_IWDG                  FALSE
#endif

/**
 * @brief   WWDG driver enable switch.
 * @details If set to @p TRUE the support for WWDG is included.
 * @note    The default is @p FALSE.
 */
#if !defined(STM32_WDG_USE_WWDG) || defined(__DOXYGEN__)
#define STM32_WDG_USE_WWDG                  FALSE
#endif
/** @} */

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if STM32_WDG_USE_IWDG && !STM32_HAS_IWDG
#error "IWDG not present in the selected device"
#endif

#if STM32_WDG_USE_WWDG && !STM32_HAS_WWDG
#error "WWDG not present in the selected device"
#endif

#if !STM32_WDG_USE_IWDG && !STM32_WDG_USE_WWDG
#error "WDG driver activated but no WDG peripheral assigned"
#endif

#if !defined(STM32_LSI_ENABLED)
#error "STM32_LSI_ENABLED not defined"
#endif

#if (STM32_WDG_USE_IWDG == TRUE) && (STM32_LSI_ENABLED == FALSE)
#error "IWDG requires LSI clock"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   Type of a structure representing an WDG driver.
 */
typedef struct WDGDriver WDGDriver;

typedef enum {
  WDG_PERIPH_IWDG = 0,
  WDG_PERIPH_WWDG = 1
} wdg_periph_t;

/**
 * @brief   Driver configuration structure.
 */
typedef struct {
#if STM32_WDG_USE_IWDG
  /**
   * @brief   Configuration of the IWDG_PR register.
   * @details See the STM32 reference manual for details.
   */
  uint32_t    pr;
  /**
   * @brief   Configuration of the IWDG_RLR register.
   * @details See the STM32 reference manual for details.
   */
  uint32_t    rlr;
#if STM32_IWDG_IS_WINDOWED || defined(__DOXYGEN__)
  /**
   * @brief   Configuration of the IWDG_WINR register.
   * @details See the STM32 reference manual for details.
   * @note    This field is not present in F1, F2, F4, L1 sub-families.
   */
  uint32_t    winr;
#endif
#endif
#if STM32_WDG_USE_WWDG
  /* WWDG fields */
  uint32_t    cfr; /**< Preconfigured CFR (prescaler + window). */
  uint8_t     cr;   /**< Reload counter value (0x40..0x7F typical). */
#endif
} WDGConfig;

/**
 * @brief   Structure representing a WDG driver.
 */
struct WDGDriver {
  /**
   * @brief   Driver state.
   */
  wdgstate_t                state;
  /**
   * @brief   Current configuration data.
   */
  const WDGConfig           *config;
  /* End of the mandatory fields.*/
#if STM32_WDG_USE_IWDG || STM32_WDG_USE_WWDG
  wdg_periph_t              type;   /**< Which peripheral this driver controls. */
  union {
  #if STM32_WDG_USE_IWDG
  /**
   * @brief   Pointer to the IWDG registers block.
   */
    IWDG_TypeDef           *iwdg;
  #endif
  #if STM32_WDG_USE_WWDG
  /**
   * @brief   Pointer to the IWDG registers block.
   */
    WWDG_TypeDef           *wwdg;
  #endif
  } reg;
#endif
};

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if STM32_WDG_USE_IWDG && !defined(__DOXYGEN__)
extern WDGDriver WDGD1;   /* IWDG instance */
#endif
#if STM32_WDG_USE_WWDG && !defined(__DOXYGEN__)
extern WDGDriver WDGD2;   /* WWDG instance */
#endif

#ifdef __cplusplus
extern "C" {
#endif
  void wdg_lld_init(void);
  void wdg_lld_start(WDGDriver *wdgp);
  void wdg_lld_stop(WDGDriver *wdgp);
  void wdg_lld_reset(WDGDriver *wdgp);
#ifdef __cplusplus
}
#endif

#endif /* HAL_USE_WDG == TRUE */

#endif /* HAL_WDG_LLD_H */
