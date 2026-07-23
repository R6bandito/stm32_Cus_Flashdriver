#ifndef __CUS_FLASH_RTOS_H__
#define __CUS_FLASH_RTOS_H__


#if (0)

    #define CUS_FLASH_SYS_ON

    #include <stdbool.h>

    /* Include FreeRTOS header as an example. */
    /* Replace with the appropriate header for your actual environment.  */
    #include "FreeRTOS.h"
    #include "task.h"

    /* Plz map to the appropriate critical section per your RTOS. */
    #define CUS_FLASH_ENTER_CRITICAL()          taskENTER_CRITICAL()
    #define CUS_FLASH_EXIT_CRITICAL()           taskEXIT_CRITICAL()

    /* These two APIs must be implemented when CUS_FLASH_USE_SYS is enabled. Empty weak defaults are provided as fallback. */
    __weak bool Cus_Flash_SYS_Lock( void );
    __weak void Cus_Flash_SYS_Unlock( void );

#endif 


#endif /* __CUS_FLASH_RTOS_H__ */

