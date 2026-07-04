#include "Cus_Flash.h"


#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)
    __weak void 
    Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage )
    {
        UNUSED(ppPage);
        for( ; ; );
    }

    __weak void 
    Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage )
    {
        UNUSED(pPage);
        for( ; ; );
    }
#endif 



#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)

    __weak void 
    Cus_FLASH_ARTEnableFailed_Hook( uint32_t ACR_ConfigWord )
    {
        UNUSED(ACR_ConfigWord);
    }

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */



__weak void 
Cus_FLASH_UnlockFailed_Hook( void )
{
	for( ; ; );
}



__weak void 
Cus_FLASH_LockFailed_Hook( void )
{
	for( ; ; );
}



__weak void 
Cus_FLASH_EraseFailed_Hook( void )
{
	for( ; ; );
}



__weak void 
Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize )
{
	UNUSED(pData);
	UNUSED(BufferSize);
	for( ; ; );
}




