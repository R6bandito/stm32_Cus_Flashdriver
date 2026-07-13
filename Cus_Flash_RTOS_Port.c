#include "./Cus_Flash_RTOS_Port.h"


#if defined (CUS_FLASH_SYS_ON)

    __weak bool Cus_Flash_SYS_Lock( void )
    {
        return true;
    }

    __weak void Cus_Flash_SYS_Unlock( void )
    {
        __nop();
    }

#endif /* CUS_FLASH_SYS_ON */


