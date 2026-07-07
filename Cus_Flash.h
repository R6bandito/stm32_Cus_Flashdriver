#ifndef __CUS_FALSH_H__
#define __CUS_FALSH_H__



/* ****************************************************** */
	#define DEVICE_STM32F1xx          			(0)
	#define DEVICE_STM32F4xx          			(1)
		#if (DEVICE_STM32F4xx)
			#define DEVICE_FLASH_TOTAL_SIZE		(256UL * 1024UL)
			#warning "Please change DEVICE_FLASH_TOTAL_SIZE to your acutal value. And the format like this: (e.g., 2048*1024 for F42x/F43x)!"
		#endif /* DEVICE_STM32F4xx */

	#define CUS_FLASH_USE_MANAGER				(1)
		#if (CUS_FLASH_USE_MANAGER)
			#define CUS_MANAGER_MAGIC			(0xBEEFFEEBUL)
			#define FLASH_MGR_MAX_RECORDS		(64)	/* Plz Adjust this value to match your actual situation. */
		#endif /* CUS_FLASH_USE_MANAGER */

	#define CUS_MEM_ALIGNED						(4UL)
/* ****************************************************** */


#if (DEVICE_STM32F1xx) && (DEVICE_STM32F4xx)
	#error "DEVICE_STM32F1xx & DEVICE_STM32F4xx can't be TRUE!" 
#endif /* (DEVICE_STM32F1xx) && (DEVICE_STM32F4xx) */


#if (DEVICE_STM32F1xx)
  #include "stm32f1xx_hal.h"
#elif (DEVICE_STM32F4xx)
  #include "stm32f4xx_hal.h"
#endif 

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


typedef enum Cus_Flash_State
{
  CUS_FLASH_OK,
  CUS_FLASH_TIMEOUT,
  CUS_FLASH_BUSY,
  CUS_FLASH_ERROR,
  CUS_FLASH_PARAMETER,
  CUS_FLASH_PROGRAM_WRPRTERR,
  CUS_FLASH_PROGRAM_PGERR,
  CUS_FLASH_ALIGNED_ERR,
  CUS_FLASH_NOT_ERASED,
  CUS_FLASH_VERIFY_ERR,
  CUS_FLASH_OVFLW_ERR,

} Cus_Flash_State_t;


/* *********************** Define ************************ */
#if (DEVICE_STM32F1xx)
	#define SYSTEMCLOCK_24Mhz           (24000000UL)
	#define SYSTEMCLOCK_48Mhz           (48000000UL)
	#define SYSTEMCLOCK_72Mhz           (72000000UL)
	#define FLASH_SIZE_REG              (*((volatile uint16_t *)0x1FFFF7E0UL)) 
#endif // DEVICE_STM32F1xx

#if (DEVICE_STM32F4xx)
	#define SYSTEMCLOCK_30Mhz           (30000000UL)
	#define SYSTEMCLOCK_60Mhz           (60000000UL)
	#define SYSTEMCLOCK_90Mhz           (90000000UL)
	#define SYSTEMCLOCK_120Mhz          (120000000UL)
	#define SYSTEMCLOCK_150Mhz          (150000000UL)
	#define SYSTEMCLOCK_168Mhz          (168000000UL)
	#define FLASH_SIZE_REG              (*((volatile uint16_t *)0x1FFF7A22UL))

	#define CUS_FLASH_SECTOR_16K		(16UL * 1024UL)
	#define CUS_FLASH_SECTOR_64K		(64UL * 1024UL)
	#define CUS_FLASH_SECTOR_128K		(128UL * 1024UL)

	#define CUS_CHECK_THRESHOLD			(512UL)
	#define CUS_CHECK_INTERVAL			(128UL)
#endif // DEVICE_STM32F4xx

#define FLASH_KEYR_KEY1               	(0x45670123UL)
#define FLASH_KEYR_KEY2               	(0xCDEF89ABUL)
#define FLASH_ERASE_TIMEOUT_MS        	(100U)
#define FLASH_SIZE_BYTES              	(FLASH_SIZE_REG * 1024UL)
#define FLASH_END_ADDR                	(FLASH_BASE + FLASH_SIZE_BYTES)
/* ******************************************************* */


/* ----------------------------------------------------------- */
#if (CUS_FLASH_USE_MANAGER)
typedef struct 
{
	/*  */
	uint16_t dataType;
	uint32_t dataSize;
	uint32_t dataStartAddr;
	char dataDesc[16];

} Cus_Flash_desc_t;

#endif /* CUS_FLASH_USE_MANAGER */
/* ----------------------------------------------------------- */


/* ----------------------------------------------------------- */
#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)

	#define FLASH_BYTES_PER_PAGE        (1024U)


	typedef struct Cus_Flash_Page Cus_Flash_Page_t;
	struct Cus_Flash_Page
	{
		uint8_t PageDataBuffer[FLASH_BYTES_PER_PAGE];
		uint32_t PageAddress;

		void (*Reset)( Cus_Flash_Page_t *pPage );
		void (*Release)( Cus_Flash_Page_t *pPage );
	};


	Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress);
	uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount );
	uint32_t Cus_Flash_GetPageAddress( uint32_t page_index );
	uint32_t Cus_Flash_GetPageStart( uint32_t Address );
	int16_t Cus_Flash_GetPageIndex( uint32_t Address );
	uint16_t Cus_Flash_GetTotalPages( void );
	int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress );
	Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage );
	Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut );
	bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size );


	__weak void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage );
	__weak void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage );

#endif /* (FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx) */



#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)
	typedef struct 
	{
		/*  */
		uint8_t secIndex;
		uint32_t secStartAddr;
		uint32_t secSize;

	} Cus_Flash_Sector_t;


	typedef struct 
	{
		/*  */
		const Cus_Flash_Sector_t *pSector;
		uint8_t *pBuffer;
		uint32_t bufSize;
		uint32_t Offset;

		#if (CUS_FLASH_USE_MANAGER)
		char desc[16];
		uint16_t dataType;
		#endif /* CUS_FLASH_USE_MANAGER */

	} Cus_Flash_SecReq_t;

	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyAddr( uint32_t Addr );
	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyIndex( uint8_t Index );
	uint8_t Cus_Flash_GetTotalSectors( void );
	uint8_t Cus_Flash_GetRemainSectors( uint8_t Index );
	uint32_t Cus_Flash_GetSectorSize( uint8_t Index );

	void Cus_Flash_EnableART( void );

	#if (!CUS_FLASH_USE_MANAGER)
		Cus_Flash_State_t Cus_Flash_EraseSector( const Cus_Flash_Sector_t *pSector );
		uint8_t Cus_Flash_EraseSectors( const Cus_Flash_Sector_t *pSector, uint8_t eNum );
		Cus_Flash_State_t Cus_Flash_WriteSector( const Cus_Flash_SecReq_t *pRequest );
		Cus_Flash_State_t Cus_Flash_ReadSector( const Cus_Flash_SecReq_t *pReq );
	#endif /* !CUS_FLASH_USE_MANAGER */

	__weak void Cus_FLASH_ARTEnableFailed_Hook( uint32_t ACR_ConfigWord );
	__weak void Cus_FLASH_EraseSectorFailed_Hook( const Cus_Flash_Sector_t *pSector );
	__weak void Cus_FLASH_WriteSectorFailed_Hook( const Cus_Flash_Sector_t *pSector );

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */


#if (CUS_FLASH_USE_MANAGER)
	Cus_Flash_State_t Cus_FlashMgr_Init( uint32_t start_addr, uint32_t end_addr );

	__weak void Cus_FLASH_MGRBufOVFL_Hook( uint16_t TotalRecords );
#endif /* CUS_FLASH_USE_MANAGER */
/* ----------------------------------------------------------- */



/* ----------------------------------------------------------- */
  bool Cus_Flash_CalibrateLatency( void );
  void Cus_Flash_Unlock( void );
  void Cus_Flash_Lock( void );
  Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
  bool Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size );
  bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size );


  __weak void Cus_FLASH_UnlockFailed_Hook( void );
  __weak void Cus_FLASH_LockFailed_Hook( void );
  __weak void Cus_FLASH_EraseFailed_Hook( void );
  __weak void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize );
/* ----------------------------------------------------------- */


#endif // __CUS_FALSH_H__
