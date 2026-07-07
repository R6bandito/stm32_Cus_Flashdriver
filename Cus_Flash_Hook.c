#include "Cus_Flash.h"


#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)
    __weak void 
    Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage )
    {
        UNUSED(ppPage);
        __nop();
    }

    __weak void 
    Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage )
    {
        UNUSED(pPage);
        __nop();
    }
#endif 



#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)

    __weak void 
    Cus_FLASH_ARTEnableFailed_Hook( uint32_t ACR_ConfigWord )
    {
        UNUSED(ACR_ConfigWord);
    }


    __weak void 
    Cus_FLASH_EraseSectorFailed_Hook( const Cus_Flash_Sector_t *pSector )
    {
        UNUSED(pSector);
    }


    __weak void 
    Cus_FLASH_WriteSectorFailed_Hook( const Cus_Flash_Sector_t *pSector )
    {
        UNUSED(pSector);
    }

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */



#if (CUS_FLASH_USE_MANAGER)

    __weak void 
    Cus_FLASH_MGRBufOVFL_Hook( uint16_t TotalRecords )
    {
        UNUSED(TotalRecords);
    }

#endif /* #if (CUS_FLASH_USE_MANAGER) */



__weak void 
Cus_FLASH_UnlockFailed_Hook( void )
{
    __nop();
}



__weak void 
Cus_FLASH_LockFailed_Hook( void )
{
    __nop();
}



__weak void 
Cus_FLASH_EraseFailed_Hook( void )
{
    __nop();
}



__weak void 
Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize )
{
	UNUSED(pData);
	UNUSED(BufferSize);
    __nop();
}




