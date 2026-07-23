#ifndef __CUS_FALSH_H__
#define __CUS_FALSH_H__



/* ****************************************************** */
	#define DEVICE_STM32F1xx          			(0)
		#if (DEVICE_STM32F1xx)
			#define CUS_FLASH_BYTE_PER_PAGE		(2048UL)
			#warning "Please change CUS_FLASH_BYTE_PER_PAGE to your acutal value."
		#endif 

	#define DEVICE_STM32F4xx          			(1)
		#if (DEVICE_STM32F4xx)
			#define DEVICE_FLASH_TOTAL_SIZE		(256UL * 1024UL)
			#warning "Please change DEVICE_FLASH_TOTAL_SIZE to your acutal value. And the format like this: (e.g., 2048*1024 for F42x/F43x)!"
		#endif /* DEVICE_STM32F4xx */

	#define CUS_FLASH_USE_MANAGER				(1)
		#if (CUS_FLASH_USE_MANAGER)
			#define CUS_MANAGER_MAGIC			(0xBEEFFEEBUL)
			#define FLASH_MGR_MAX_RECORDS		(64)	/* Plz Adjust this value to match your actual situation. */
			#define FLASH_MGR_MAX_INSTANCES		(4)
		#endif /* CUS_FLASH_USE_MANAGER */

	#define CUS_FLASH_USE_SYS					(0)
		#if (CUS_FLASH_USE_SYS)
			#include "./Cus_Flash_RTOS_Port.h"

			#ifndef CUS_FLASH_SYS_ON
				#error "Plz enable RTOS in Cus_Flash_RTOS_Port.h."
			#endif /* CUS_FLASH_SYS_ON */
		#endif /* CUS_FLASH_USE_SYS */
/* ****************************************************** */


#if (DEVICE_STM32F1xx) && (DEVICE_STM32F4xx)
	#error "DEVICE_STM32F1xx & DEVICE_STM32F4xx can't be TRUE!" 
#endif /* (DEVICE_STM32F1xx) && (DEVICE_STM32F4xx) */


#if (DEVICE_STM32F1xx)
  #include "stm32f1xx.h"
#elif (DEVICE_STM32F4xx)
  #include "stm32f4xx.h"
#endif 

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * CUS_FLASH_GET_TICK — Get system millisecond timestamp.
 *
 * User may override. Default uses the Cortex-M DWT cycle counter
 * (zero peripheral dependency; call Cus_Flash_SYS_TickInit() at startup).
 * Examples:
 *   #define CUS_FLASH_GET_TICK()  HAL_GetTick()
 *   #define CUS_FLASH_GET_TICK()  xTaskGetTickCount()
 */
#ifndef CUS_FLASH_GET_TICK
	#define CUS_FLASH_GET_TICK()  Cus_Flash_SYS_GetTick()
	void Cus_Flash_SYS_TickInit( void );
	uint32_t Cus_Flash_SYS_GetTick( void );
#endif


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
	CUS_FLASH_NOSPACE_ERR,
	CUS_FLASH_NOT_FOUND,
	CUS_FLASH_OVERLAP_ERR,
	CUS_FLASH_LOW_VOLTAGE_ERR,
	CUS_FLASH_MUTEX_ERR,

} Cus_Flash_State_t;


typedef enum Cus_Flash_PVD
{
	CUS_FLASH_PVD_LEVEL0 = 0,
	CUS_FLASH_PVD_LEVEL1,
	CUS_FLASH_PVD_LEVEL2,
	CUS_FLASH_PVD_LEVEL3,
	CUS_FLASH_PVD_LEVEL4,
	CUS_FLASH_PVD_LEVEL5,
	CUS_FLASH_PVD_LEVEL6,
	CUS_FLASH_PVD_LEVEL7,

} Cus_Flash_PVD_t;


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
#define FLASH_ERASE_TIMEOUT_MS        	(3000U)
#define FLASH_SIZE_BYTES              	(FLASH_SIZE_REG * 1024UL)
#define FLASH_END_ADDR                	(FLASH_BASE + FLASH_SIZE_BYTES)
/* ******************************************************* */


/* ----------------------------------------------------------- */
#if (CUS_FLASH_USE_MANAGER)

	typedef struct 
	{
		/**
			* External representation of a stored record's metadata.
			* Returned by Manager query APIs for the upper layer to inspect.
		 */
		uint16_t dataType;			/**< Application-defined data type tag. */
		uint32_t dataSize;			/**< Size of the user data block in bytes. */
		uint32_t dataStartAddr;		/**< Absolute Flash address where the user data begins. */
		uint32_t dataIndexInPoll;	/**< The Record index in the gs_records table. */
		char dataDesc[16];			/**< Human-readable description string. */

	} Cus_Flash_desc_t;

#endif /* CUS_FLASH_USE_MANAGER */
/* ----------------------------------------------------------- */


/* ----------------------------------------------------------- */
#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)

	typedef struct 
	{
		uint8_t *pBuffer;
		uint32_t bufSize;
		uint32_t Offset;
		uint32_t PageStartAddress;

		#if (CUS_FLASH_USE_MANAGER)
		char desc[16];							/**< Human-readable description stored in the on-flash header. */
		uint16_t dataType;						/**< Application-defined data type tag. */
		#endif /* CUS_FLASH_USE_MANAGER */

	} Cus_Flash_PageReq_t;


	Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress);
	int16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount );
	uint32_t Cus_Flash_GetPageAddress( uint32_t page_index );
	uint32_t Cus_Flash_GetPageStart( uint32_t Address );
	int16_t Cus_Flash_GetPageIndex( uint32_t Address );
	int16_t Cus_Flash_GetTotalPages( void );
	int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress );

	#if (!CUS_FLASH_USE_MANAGER)
		Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_PageReq_t *pPage );
		Cus_Flash_State_t Cus_Flash_ReadPage( Cus_Flash_PageReq_t *pPage );
	#endif 

	__weak void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_PageReq_t *pPage );

#endif /* (FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx) */



#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)

	typedef struct 
	{
		/*
			* Describes a single Flash sector: its index, base address, and size.
			* Used by the sector-mode driver for erase and address lookup.
		*/
		uint8_t secIndex;   	/**< Zero-based hardware sector number (used for SNB register). */
		uint32_t secStartAddr;	/**< Absolute start address of this sector in Flash. */
		uint32_t secSize;		/**< Total size of this sector in bytes. */

	} Cus_Flash_Sector_t;

	typedef struct 
	{
		/**
			 * Unified request packet for sector-mode read and write operations.
			 * When Manager is enabled, optional desc/dataType fields are included
			 * for automatic header generation.
		 */
		const Cus_Flash_Sector_t *pSector;		/**< Target sector (The Sectors that you want to handle). */
		uint8_t *pBuffer;						/**< User data buffer for read destination or write source. */
		uint32_t bufSize;						/**< Size of the user data in bytes. */
		uint32_t Offset;						/**< Byte offset from sector start to begin the operation. */

		#if (CUS_FLASH_USE_MANAGER)
		char desc[16];							/**< Human-readable description stored in the on-flash header. */
		uint16_t dataType;						/**< Application-defined data type tag. */
		#endif /* CUS_FLASH_USE_MANAGER */

	} Cus_Flash_SecReq_t;

	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyAddr( uint32_t Addr );
	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyIndex( uint8_t Index );
	uint8_t Cus_Flash_GetTotalSectors( void );
	uint8_t Cus_Flash_GetRemainSectors( uint8_t Index );
	uint32_t Cus_Flash_GetSectorSize( uint8_t Index );

	Cus_Flash_State_t Cus_Flash_EraseSector( const Cus_Flash_Sector_t *pSector );
	uint8_t Cus_Flash_EraseSectors( const Cus_Flash_Sector_t *pSector, uint8_t eNum );

	void Cus_Flash_EnableART( void );

	#if (!CUS_FLASH_USE_MANAGER)		
		Cus_Flash_State_t Cus_Flash_WriteSector( const Cus_Flash_SecReq_t *pRequest );
		Cus_Flash_State_t Cus_Flash_ReadSector( const Cus_Flash_SecReq_t *pReq );
	#endif /* !CUS_FLASH_USE_MANAGER */

	__weak void Cus_FLASH_ARTEnableFailed_Hook( uint32_t ACR_ConfigWord );
	__weak void Cus_FLASH_EraseSectorFailed_Hook( const Cus_Flash_Sector_t *pSector );
	__weak void Cus_FLASH_WriteSectorFailed_Hook( const Cus_Flash_Sector_t *pSector );

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */ 


#if (CUS_FLASH_USE_MANAGER)

	typedef struct 
	{
		/**
			* Request packet for appending a new record via the Manager layer.
			* The Manager handles header generation and free-space placement automatically.
		 */
		uint32_t DataType;		/**< Application-defined data type tag stored in the header. */
		char DataDesc[16];		/**< Human-readable description stored in the header. */
		uint8_t *DataBuff;		/**< Pointer to the user data to be written. */
		uint32_t DataSize;		/**< Size of the user data in bytes. */

	} Cus_FlashMgr_Req_t;


	typedef struct 
	{
		/**
			* RAM-resident index entry built during Init scanning.
			* One entry per valid on-flash record; used for all Manager queries and deletions.
		 */
		uint32_t msgStartAddr;		/**< Absolute Flash address where the user data begins. */
		uint32_t msgSize;			/**< Size of the user data in bytes. */
		uint16_t msgType;			/**< Application-defined data type tag. */
		char msgDetail[16];			/**< Human-readable description string. */
		
	} FlashMgr_Record_t;


	typedef struct 
	{
		uint32_t mgrStartAddr;
		uint32_t mgrEndAddr;
		uint32_t mgrLowestFreeAddr;
		uint32_t mgrRecordCount;
		FlashMgr_Record_t mgrRecords[FLASH_MGR_MAX_RECORDS];

	} FlashMgr_Instance_t;


	/* Debug print callback. */
	typedef void (*Cus_Flash_PrintCB)( const char *src );
	typedef bool (*Cus_Flash_KeepCB)( const FlashMgr_Record_t *record, void *ctx );


	Cus_Flash_State_t Cus_FlashMgr_Init( FlashMgr_Instance_t *instance, uint32_t start_addr, uint32_t end_addr );
	Cus_Flash_State_t Cus_FlashMgr_Append( FlashMgr_Instance_t *instance, const Cus_FlashMgr_Req_t *pReq );
	Cus_Flash_State_t Cus_FlashMgr_GetRecordByDesc( FlashMgr_Instance_t *instance, const char *desc, Cus_Flash_desc_t *pOut );
	Cus_Flash_State_t Cus_FlashMgr_DeleteByIndex( FlashMgr_Instance_t *instance, uint32_t recordIndex );
	Cus_Flash_State_t Cus_FlashMgr_DeleteByDesc( FlashMgr_Instance_t *instance, const char *desc );
	Cus_Flash_State_t Cus_FlashMgr_GetRecordCount( FlashMgr_Instance_t *instance, uint16_t *pOut );
	Cus_Flash_State_t Cus_FlashMgr_GetFreeSpace( FlashMgr_Instance_t *instance, uint32_t *pOut );
	Cus_Flash_State_t Cus_FlashMgr_EraseRegion( FlashMgr_Instance_t *instance );
	Cus_Flash_State_t Cus_FlashMgr_DeInit( FlashMgr_Instance_t *instance );
	Cus_Flash_State_t Cus_FlashMgr_DeleteAllByDesc( FlashMgr_Instance_t *instance, const char *desc, uint16_t *delCnt );
	Cus_Flash_State_t Cus_FlashMgr_DumpByDesc( FlashMgr_Instance_t *instance, const char *desc, Cus_Flash_PrintCB pcallback );
	Cus_Flash_State_t Cus_FlashMgr_DumpAll( FlashMgr_Instance_t *instance, Cus_Flash_PrintCB pcallback );
	Cus_Flash_State_t Cus_FlashMgr_Compact( FlashMgr_Instance_t *instance, Cus_Flash_KeepCB cb, void *ctx, uint8_t *backBuf, uint32_t bufSize );

	__weak void Cus_FLASH_MGRBufOVFL_Hook( uint16_t TotalRecords );

#endif /* CUS_FLASH_USE_MANAGER */
/* ----------------------------------------------------------- */



/* ----------------------------------------------------------- */
Cus_Flash_State_t Cus_Flash_PVDConfig( Cus_Flash_PVD_t PVDLevel );
void Cus_Flash_PVDSet( void );
void Cus_Flash_PVDClr( void );

bool Cus_Flash_CalibrateLatency( void );
void Cus_Flash_Unlock( void );
void Cus_Flash_Lock( void );
Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
bool Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size );
bool Cus_Flash_IsValid( uint32_t StartAddress, uint32_t Size );

__weak void Cus_FLASH_UnlockFailed_Hook( void );
__weak void Cus_FLASH_LockFailed_Hook( void );
__weak void Cus_FLASH_EraseFailed_Hook( void );
__weak void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize );
/* ----------------------------------------------------------- */


#endif // __CUS_FALSH_H__
