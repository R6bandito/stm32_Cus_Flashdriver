#include "Cus_Flash.h"


/* ****************************************************** */
bool Cus_Flash_CalibrateLatency( void );
void Cus_Flash_Unlock( void );
void Cus_Flash_Lock( void );
static uint32_t Cus_Flash_GetSpinCount( uint32_t ms );
Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
bool Cus_Flash_VerifyBuffer(uint32_t StartAddress, uint8_t *pData, uint32_t Size);
bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size );


#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)
	uint32_t Cus_Flash_GetPageAddress( uint32_t page_index );
	uint32_t Cus_Flash_GetPageStart( uint32_t Address );
	bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size );
	Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress);
	uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount );
	Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage );
	int16_t Cus_Flash_GetPageIndex( uint32_t Address );
	uint16_t Cus_Flash_GetTotalPages( void );
	int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress );
	Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut );
	static void PageStructure_Reset( Cus_Flash_Page_t *pPage );
	static void PageStructure_Release( Cus_Flash_Page_t *pPage );
#endif // FLASH_TYPEERASE_PAGES


#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)
	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyAddr( uint32_t Addr );
	const Cus_Flash_Sector_t *Cus_Flash_GetSectorbyIndex( uint8_t Index );
	uint8_t Cus_Flash_GetTotalSectors( void );
	uint8_t Cus_Flash_GetRemainSectors( uint8_t Index );
	uint32_t Cus_Flash_GetSectorSize( uint8_t Index );

	void Cus_Flash_EnableART( void );
	Cus_Flash_State_t Cus_Flash_EraseSector( const Cus_Flash_Sector_t *pSector );
	uint8_t Cus_Flash_EraseSectors( const Cus_Flash_Sector_t *pSector, uint8_t eNum );
	Cus_Flash_State_t Cus_Flash_WriteSector( const Cus_Flash_SecReq_t *pRequest );

	static Cus_Flash_State_t Cus_Flash_WaitBusy( uint32_t timeout_ms );
#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */
/* ****************************************************** */


/* ****************************************************** */
#if (CUS_FLASH_USE_MANAGER)
typedef struct 
{
	/*  */
	uint32_t Magic;
	uint32_t Size;
	uint32_t StartAddr;
	uint16_t  Type;
	uint16_t  validFlag;
	char Desc[16];

} desc_t;

#endif /* CUS_FLASH_USE_MANAGER */


#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)
	static const Cus_Flash_Sector_t SectorTable[] = 
	{
		#if ((DEVICE_FLASH_TOTAL_SIZE) >= (256UL * 1024UL))
		/* SECTOR 0 ~ 3. 16KB per sector. */
		{.secIndex = 0, .secStartAddr = FLASH_BASE, 		  .secSize = CUS_FLASH_SECTOR_16K},
		{.secIndex = 1, .secStartAddr = FLASH_BASE + 0x04000, .secSize = CUS_FLASH_SECTOR_16K},
		{.secIndex = 2, .secStartAddr = FLASH_BASE + 0x08000, .secSize = CUS_FLASH_SECTOR_16K},
		{.secIndex = 3, .secStartAddr = FLASH_BASE + 0x0C000, .secSize = CUS_FLASH_SECTOR_16K},

		/* SECTOR 4. 64KB. */
		{.secIndex = 4, .secStartAddr = FLASH_BASE + 0x10000, .secSize = CUS_FLASH_SECTOR_64K},

		/* SECTOR > 4 (BANK1). 128KB per Sector. */
		{.secIndex = 5, .secStartAddr = FLASH_BASE + 0x20000, .secSize = CUS_FLASH_SECTOR_128K},
		#endif /* 256KB FLASH. */

		#if ((DEVICE_FLASH_TOTAL_SIZE) >= (512UL * 1024UL))
		{.secIndex = 6, .secStartAddr = FLASH_BASE + 0x40000, .secSize = CUS_FLASH_SECTOR_128K},
		{.secIndex = 7, .secStartAddr = FLASH_BASE + 0x60000, .secSize = CUS_FLASH_SECTOR_128K},
		#endif /* 512KB FLASH. */

		#if ((DEVICE_FLASH_TOTAL_SIZE) >= (1024UL * 1024UL))
		{.secIndex = 8,  .secStartAddr = FLASH_BASE + 0x80000, .secSize = CUS_FLASH_SECTOR_128K},
		{.secIndex = 9,  .secStartAddr = FLASH_BASE + 0xA0000, .secSize = CUS_FLASH_SECTOR_128K},
		{.secIndex = 10, .secStartAddr = FLASH_BASE + 0xC0000, .secSize = CUS_FLASH_SECTOR_128K},
		{.secIndex = 11, .secStartAddr = FLASH_BASE + 0xE0000, .secSize = CUS_FLASH_SECTOR_128K},
		#endif /* 1MB FLASH. */

		#if ((DEVICE_FLASH_TOTAL_SIZE) >= (2048UL * 1024UL))
		/* BANK2. Refer BANK1. */
		{.secIndex = 12, .secStartAddr = FLASH_BASE + 0x100000, .secSize = CUS_FLASH_SECTOR_16K},  // 0x0810 0000
		{.secIndex = 13, .secStartAddr = FLASH_BASE + 0x104000, .secSize = CUS_FLASH_SECTOR_16K},  // 0x0810 4000
		{.secIndex = 14, .secStartAddr = FLASH_BASE + 0x108000, .secSize = CUS_FLASH_SECTOR_16K},  // 0x0810 8000
		{.secIndex = 15, .secStartAddr = FLASH_BASE + 0x10C000, .secSize = CUS_FLASH_SECTOR_16K},  // 0x0810 C000
		{.secIndex = 16, .secStartAddr = FLASH_BASE + 0x110000, .secSize = CUS_FLASH_SECTOR_64K},  // 0x0811 0000
		{.secIndex = 17, .secStartAddr = FLASH_BASE + 0x120000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x0812 0000
		{.secIndex = 18, .secStartAddr = FLASH_BASE + 0x140000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x0814 0000
		{.secIndex = 19, .secStartAddr = FLASH_BASE + 0x160000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x0816 0000
		{.secIndex = 20, .secStartAddr = FLASH_BASE + 0x180000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x0818 0000
		{.secIndex = 21, .secStartAddr = FLASH_BASE + 0x1A0000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x081A 0000
		{.secIndex = 22, .secStartAddr = FLASH_BASE + 0x1C0000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x081C 0000
		{.secIndex = 23, .secStartAddr = FLASH_BASE + 0x1E0000, .secSize = CUS_FLASH_SECTOR_128K}, // 0x081E 0000
		#endif /* 2MB FLASH. */
	};

	static const uint8_t gsc_secTotalCount = (sizeof(SectorTable) / sizeof(SectorTable[0]));

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */
/* ****************************************************** */


void 
Cus_Flash_Lock( void )
{
	uint32_t FLASH_CR_RegTemp = FLASH->CR;

	if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) != 0 )    return;   /* Has already be locked. Return. */

	uint32_t timeout = Cus_Flash_GetSpinCount(100);
	while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout )
	{
		__nop();
	}
	if ( !timeout )
	{
		/* Timeout waiting for Flash BSY bit to clear. Bus is still busy. */
		Cus_FLASH_LockFailed_Hook();
		return;
	}

	/* FLASH_CR Lock bit set. */
	FLASH->CR |= FLASH_CR_LOCK_Msk;

	FLASH_CR_RegTemp = FLASH->CR;
	if ( !(FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) )
	{
		/* Lock failed! FLASH_CR Lock bit still reset.(write/read-back dismatch!) */
		Cus_FLASH_LockFailed_Hook();
	}
}



void 
Cus_Flash_Unlock( void )
{
	uint32_t FLASH_CR_RegTemp = FLASH->CR;

	if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) == 0 )  return;   /* Has already be unlocked. Return. */

	uint32_t timeout = Cus_Flash_GetSpinCount(100);
	while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout )
	{
		__nop();
	}
	if ( !timeout )
	{
		/* Timeout waiting for Flash BSY bit to clear. Bus is still busy. */
		Cus_FLASH_UnlockFailed_Hook();
		return;
	}

	/* Start Flash unlock sequence by writing KEY1 to FLASH_KEYR. */ 
	FLASH->KEYR = FLASH_KEYR_KEY1;    
	__DSB();

	/* Writing the last KEY Code(KEY2) to FLASH_KEYR. */
	FLASH->KEYR = FLASH_KEYR_KEY2;    

	FLASH_CR_RegTemp = FLASH->CR;
	if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) )   
	{
		/* Unlock failed! FLASH_CR Lock bit still set. (write/read-back dismatch!) */
		Cus_FLASH_UnlockFailed_Hook();
	}
}



bool 
Cus_Flash_CalibrateLatency( void )
{
	SystemCoreClockUpdate();
	uint32_t desire_Latancy = ((FLASH->ACR) & 0x07UL);

	#if (DEVICE_STM32F1xx)
		if ( (SystemCoreClock > 0) && (SystemCoreClock <= SYSTEMCLOCK_24Mhz) )
		{
			if ( desire_Latancy == 0x00 )   return true;
			else  desire_Latancy = 0x00;
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_24Mhz) && (SystemCoreClock <= SYSTEMCLOCK_48Mhz) )
		{
			if ( desire_Latancy == 0x01 )   return true;
			else desire_Latancy = 0x01;
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_48Mhz) && (SystemCoreClock <= SYSTEMCLOCK_72Mhz) )
		{
			if ( desire_Latancy == ((0x01UL) << 1) )  return true;
			else desire_Latancy = ((0x01UL) << 1);
		}
		else 
		{
			return false;               // 时钟错误. 不在范围内.
		}
	#endif // DEVICE_STM32F1xx

	#if (DEVICE_STM32F4xx)
		if ( (SystemCoreClock > 0) && (SystemCoreClock <= SYSTEMCLOCK_30Mhz) )
		{
			if ( desire_Latancy == 0x00 )   return true;
			else desire_Latancy = 0x00;
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_30Mhz) && (SystemCoreClock <= SYSTEMCLOCK_60Mhz) )
		{
			if ( desire_Latancy == 0x01 )   return true;
			else desire_Latancy = 0x01;
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_60Mhz) && (SystemCoreClock <= SYSTEMCLOCK_90Mhz) )
		{
			if ( desire_Latancy == ((0x01UL) << 1) )  return true;
			else desire_Latancy = ((0x01UL) << 1);
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_90Mhz) && (SystemCoreClock <= SYSTEMCLOCK_120Mhz) )
		{
			if ( desire_Latancy == ((0x01UL) << 1) | 0x01UL )  return true;
			else desire_Latancy = ((0x01UL) << 1) | 0x01UL;
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_120Mhz) && (SystemCoreClock <= SYSTEMCLOCK_150Mhz) )
		{
			if ( desire_Latancy == ((0x01UL) << 2) )  return true;
			else desire_Latancy = ((0x01UL) << 2);
		}
		else if ( (SystemCoreClock > SYSTEMCLOCK_150Mhz) && (SystemCoreClock <= SYSTEMCLOCK_168Mhz) )
		{
			if ( desire_Latancy == ((0x01UL) << 2) | 0x01UL )  return true;
			else desire_Latancy = ((0x01UL) << 2) | 0x01UL;
		}
		else 
		{
			return false;       // 时钟错误. 不在范围内.
		}
	#endif // DEVICE_STM32F4xx

	__disable_irq();
	FLASH->ACR = (FLASH->ACR & ~0x07UL) | desire_Latancy;
	__enable_irq();

	return ((FLASH->ACR & 0x07UL) == desire_Latancy);
}



/**
 * @brief 根据系统时钟计算自旋等待的循环次数（粗略估算）
 * @param ms 超时毫秒数
 * @return 循环次数（每个循环约 5~6 个指令周期）
 */
static uint32_t 
Cus_Flash_GetSpinCount( uint32_t ms )
{
	// 估算每个循环消耗的 CPU 周期数.
	const uint32_t cycles_per_loop = 5;

	uint32_t sysclk = SystemCoreClock;

	uint32_t total_cycles = (ms * sysclk) / 1000;

	return total_cycles / cycles_per_loop;
}



Cus_Flash_State_t 
Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size )
{
	if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || !pData || Buffer_Size == 0 )   
		return CUS_FLASH_PARAMETER;

	if ( (StartAddress & 0x01UL) != 0 )   // 地址未半字对齐. 
	{
		return CUS_FLASH_PARAMETER;
	}

	uint8_t is_NeedExtraEdit = 0;
	if ( (Buffer_Size % 2) != 0 )  // 需要写入的数据为奇数(非2的偶数倍). 最后多一个字节需要特殊处理.   
	{
		is_NeedExtraEdit = 1;
	}

	__disable_irq();
	Cus_Flash_Unlock();

	uint32_t addr = StartAddress;
	uint8_t *pSrc = pData;
	uint32_t size = Buffer_Size;

	while( size >= 2 )
	{
		if ( HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, addr, *(uint16_t *)pSrc) != HAL_OK )
		{
			__enable_irq();
			Cus_Flash_Lock();
			return CUS_FLASH_ERROR;
		}

		addr += 2;
		pSrc += 2;
		size -= 2;
	}

	if ( is_NeedExtraEdit )
	{
		// 还剩最后两字节需要处理.
		uint16_t last_16bit = 0;
		last_16bit = *pSrc; 
		if ( HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, addr, last_16bit) != HAL_OK )
		{
			__enable_irq();
			Cus_Flash_Lock();
			return CUS_FLASH_ERROR;
		}
	}

	__enable_irq();
	Cus_Flash_Lock();
	return CUS_FLASH_OK;
}



bool 
Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size )
{
	if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || !pData || Size == 0 )  return false;

	for( uint32_t i = 0; i < Size; i++ )
	{
		if ( *((uint8_t *)StartAddress + i) != pData[i] )   
		{
			Cus_FLASH_VerifyBufferFailed_Hook(StartAddress, pData, Size);

			return false;
		}
	}

	return true;
}



bool 
Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size )
{
	if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || Size == 0 )  return false;

	for( uint32_t i = 0; i < Size; i++ )
	{
		if ( *((uint8_t *)StartAddress + i) != 0xFF )   return false;
	}

	return true;
}



#if defined(FLASH_TYPEERASE_PAGES) && (DEVICE_STM32F1xx)

	Cus_Flash_State_t 
	Cus_Flash_ErasePage( uint32_t PageAddress )
	{
		if ( (PageAddress % 1024) != 0)   return CUS_FLASH_PARAMERR;     // 非页对齐.

		if ( (PageAddress < FLASH_BASE) || (PageAddress >= FLASH_END_ADDR) )  return CUS_FLASH_PARAMERR;

		if ( (FLASH->SR & FLASH_SR_BSY_Msk) != 0 )  return CUS_FLASH_BUSY;  // 已有闪存操作正在进行. 返回BUSY.
		__disable_irq();                                // 关中断.
		Cus_Flash_Unlock();

		FLASH->CR = ((FLASH->CR) | (0x01UL << FLASH_CR_PER_Pos));       // 设置 PER 位为1.
		FLASH->AR = (uint32_t)PageAddress;                     // 设置要擦除的页.
		FLASH->CR = ((FLASH->CR) | (0x01UL << FLASH_CR_STRT_Pos));      // 设置 STRT 位为1.

		uint32_t timeout = Cus_Flash_GetSpinCount(FLASH_ERASE_TIMEOUT_MS);
		while( (FLASH->SR & FLASH_SR_BSY_Msk) != 0 && timeout-- )
		{
			__NOP();
		}
		__enable_irq();

		if ( timeout == 0 )
		{
			FLASH->CR &= ~FLASH_CR_PER_Msk;
			Cus_FLASH_EraseFailed_Hook();
			Cus_Flash_Lock();
			return CUS_FLASH_TIMEOUT;
		}

		if ( FLASH->SR & (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk) )   // 检查错误标志.
		{
			if ( (FLASH->SR & (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk)) == (FLASH_SR_WRPRTERR_Msk | FLASH_SR_PGERR_Msk) ) // 双重错误.
			{
				Cus_FLASH_EraseFailed_Hook();
				FLASH->SR |= ((0x01UL << FLASH_SR_WRPRTERR_Pos) | (0x01UL << FLASH_SR_PGERR_Pos));
				FLASH->CR &= ~FLASH_CR_PER_Msk;
				Cus_Flash_Lock();
				return CUS_FLASH_ERROR;
			}

			int8_t flag = 0;
			if ( FLASH->SR & FLASH_SR_WRPRTERR_Msk )
			{
				flag = 1;
				Cus_FLASH_EraseFailed_Hook();
			}

			if ( FLASH->SR & FLASH_SR_PGERR_Msk )
			{
				flag = -1;
				Cus_FLASH_EraseFailed_Hook();
			}

			FLASH->SR = (FLASH->SR | (0x01UL << FLASH_SR_WRPRTERR_Pos) | (0x01UL << FLASH_SR_PGERR_Pos));   // 清除错误标志.
			FLASH->CR &= ~FLASH_CR_PER_Msk;
			Cus_Flash_Lock();
			if ( flag > 0 )  return CUS_FLASH_PROGRAM_WRPRTERR;
			else if ( flag < 0 )  return CUS_FLASH_PROGRAM_PGERR;
		}

		FLASH->CR &= ~FLASH_CR_PER_Msk;
		Cus_Flash_Lock();
		return CUS_FLASH_OK;
	}



	uint32_t 
	Cus_Flash_GetPageAddress( uint32_t page_index )
	{
		if ( (FLASH_BASE + page_index * FLASH_BYTES_PER_PAGE) >= FLASH_END_ADDR )   return 0;

		return (FLASH_BASE + ( page_index * FLASH_BYTES_PER_PAGE ));
	}



	uint32_t 
	Cus_Flash_GetPageStart( uint32_t Address )
	{
		if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return 0;
		
		if ( (Address % FLASH_BYTES_PER_PAGE) == 0 )  return 0;   // 已经属于整页地址. 空调用.

		return Cus_Flash_GetPageAddress( ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE) );
	}



	Cus_Flash_State_t 
	Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut )
	{
		/* 关于Factory_GetPageControlBlock:
				该API用于向用户提供一个动态分配的 Cus_Flash_Page_t 结构，用于后续的整页写入等操作. 由于 Cus_Flash_Page_t 所占空间较大，建议全局仅分配一个 Cus_Flash_Page_t 
				并且当使用完一次后，手动调用成员 Reset(). 便于下次使用.*/

		if ( !pPageOut )    return CUS_FLASH_ERROR;

		#warning "If malloc failed. Please check your heap size config."
		Cus_Flash_Page_t *pReturn = (Cus_Flash_Page_t *)malloc(sizeof(Cus_Flash_Page_t));
		if ( !pReturn )
		{
			Cus_FLASH_PageStructMallocFailed_Hook( pPageOut );

			return CUS_FLASH_ERROR;
		}

		*pPageOut = pReturn;
		memset((*pPageOut)->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
		(*pPageOut)->PageAddress = 0;
		(*pPageOut)->Reset = PageStructure_Reset;
		(*pPageOut)->Release = PageStructure_Release;

		return CUS_FLASH_OK;
	}



	Cus_Flash_State_t 
	Cus_Flash_WritePage( Cus_Flash_Page_t *pPage )
	{
		if ( !pPage || pPage->PageAddress < FLASH_BASE || pPage->PageAddress > FLASH_END_ADDR || ((pPage->PageAddress) % FLASH_BYTES_PER_PAGE) != 0 )
			return CUS_FLASH_PARAMERR;

		if ( Cus_Flash_ErasePage( pPage->PageAddress ) != CUS_FLASH_OK )
		{
			return CUS_FLASH_ERROR;
		}

		if ( Cus_Flash_WriteBuffer(pPage->PageAddress, pPage->PageDataBuffer, FLASH_BYTES_PER_PAGE) != CUS_FLASH_OK )
		{
			Cus_FLASH_PageWriteFailed_Hook( pPage );

			return CUS_FLASH_ERROR;
		}

		if ( !Cus_Flash_VerifyBuffer(pPage->PageAddress, pPage->PageDataBuffer, FLASH_BYTES_PER_PAGE) )
		{
			return CUS_FLASH_ERROR;
		}

		return CUS_FLASH_OK;
	}



	static void 
	PageStructure_Reset( Cus_Flash_Page_t *pPage )
	{
		if ( !pPage )   return;

		pPage->PageAddress = 0;
		memset(pPage->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
	}



	static void 
	PageStructure_Release( Cus_Flash_Page_t *pPage )
	{
		/* 注意！ 传入的Cus_Flash_Page_t *pPage 必须是经由 Factory_GetPageControlBlock 创建并初始化的.否则将会出现释放错误. */
		if ( !pPage )   return;

		memset(pPage, 0, sizeof(Cus_Flash_Page_t));

		free(pPage);
	}



	bool 
	Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size )
	{
		if ( PageAddress < FLASH_BASE || PageAddress > FLASH_END_ADDR || !pOutBuffer || Size == 0 )  return false;

		uint32_t Read_count = 0;
		if ( Size <= FLASH_BYTES_PER_PAGE )  
			Read_count = Size;
		else  
			Read_count = FLASH_BYTES_PER_PAGE;


		for( uint32_t j = 0; j < Read_count; j++ )
		{
			pOutBuffer[j] = *((uint8_t *)PageAddress + j);
		}

		int32_t remaining = Size - FLASH_BYTES_PER_PAGE;   // 缓冲区给大了，已经读完一页了，剩下空间以0填充.
		if (remaining > 0) 
		{
			memset(&pOutBuffer[FLASH_BYTES_PER_PAGE], 0, remaining);
		}

		return true;
	}



	int16_t 
	Cus_Flash_GetPageIndex( uint32_t Address )
	{
		if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return -1;

		return ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE);
	}



	uint16_t 
	Cus_Flash_GetTotalPages( void )
	{
		return ( (FLASH_END_ADDR - FLASH_BASE) / FLASH_BYTES_PER_PAGE );
	}



	int32_t 
	Cus_Flash_GetRemainPages( uint32_t PageAddress )
	{
		if ( PageAddress < FLASH_BASE || PageAddress > FLASH_END_ADDR || (PageAddress % FLASH_BYTES_PER_PAGE) != 0 )   
			return -1;

		return ( (FLASH_END_ADDR - PageAddress) / FLASH_BYTES_PER_PAGE );
	}



	uint16_t 
	Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount )
	{
		if ( PageStartAddress < FLASH_BASE || PageStartAddress > FLASH_END_ADDR || (PageStartAddress % FLASH_BYTES_PER_PAGE) != 0 )   return 0;

		uint16_t PagesRemain = Cus_Flash_GetRemainPages(PageStartAddress);
		uint16_t PagesWaitToBeErase = PageCount;

		if ( PagesWaitToBeErase > PagesRemain )   return 0;

		int16_t CurrectPage = Cus_Flash_GetPageIndex(PageStartAddress);
		uint16_t SuccessFulEraseCount = 0;

		for( uint16_t i = 0; i < PagesWaitToBeErase; i++ )
		{
			if ( Cus_Flash_ErasePage(Cus_Flash_GetPageAddress((uint32_t)CurrectPage + i)) != CUS_FLASH_OK )
			{
				return SuccessFulEraseCount;
			}

			SuccessFulEraseCount++;
		}

		return PagesWaitToBeErase;
	}

#endif // FLASH_TYPEERASE_PAGES



#if defined(FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx)

	static Cus_Flash_State_t 
	Cus_Flash_WaitBusy( uint32_t timeout_ms )
	{
		uint32_t timeout = Cus_Flash_GetSpinCount(timeout_ms);
		while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout ) { __nop(); }

		static const uint32_t errorMask = FLASH_SR_PGAERR_Msk | FLASH_SR_PGPERR_Msk | FLASH_SR_PGSERR_Msk | FLASH_SR_WRPERR_Msk;
		if ( FLASH->SR & FLASH_SR_BSY_Msk ) return CUS_FLASH_TIMEOUT;

		if ( FLASH->SR & errorMask ) 
		{
			/* Clear error flag and return. */
			FLASH->SR |= errorMask;
			return CUS_FLASH_ERROR;
		}

		return CUS_FLASH_OK;
	}



	const Cus_Flash_Sector_t *
	Cus_Flash_GetSectorbyAddr( uint32_t Addr )
	{
		if ( (Addr < FLASH_BASE) || (Addr >= FLASH_END_ADDR) )	return NULL;

		for( uint8_t index = 0; index < gsc_secTotalCount; index++ )
		{
			uint32_t end_addr = SectorTable[index].secStartAddr + SectorTable[index].secSize;
			uint32_t start_addr = SectorTable[index].secStartAddr;
			if ( (Addr >= start_addr) && (Addr < end_addr) )
			{
				/* Find relevant sector. */
				return (const Cus_Flash_Sector_t *)&SectorTable[index];
			}
		}

		return NULL;
	}



	const Cus_Flash_Sector_t *
	Cus_Flash_GetSectorbyIndex( uint8_t Index )
	{
		if ( Index >= gsc_secTotalCount )	return NULL;

		return (const Cus_Flash_Sector_t *)&SectorTable[Index];
	}



	uint8_t 
	Cus_Flash_GetTotalSectors( void )
	{
		return gsc_secTotalCount;
	}



	uint32_t 
	Cus_Flash_GetSectorSize( uint8_t Index )
	{
		if ( Index >= gsc_secTotalCount )	return 0;

		return SectorTable[Index].secSize;
	}



	uint8_t 
	Cus_Flash_GetRemainSectors( uint8_t Index )
	{
		if ( Index >= gsc_secTotalCount )	return 0;

		return ((gsc_secTotalCount - 1) - Index);
	}



	void 
	Cus_Flash_EnableART( void )
	{
		uint32_t acrReg = FLASH->ACR;
		uint32_t currentHCLK = HAL_RCC_GetHCLKFreq();
		if ( (currentHCLK > SYSTEMCLOCK_60Mhz) && ((acrReg & FLASH_ACR_LATENCY_Msk) < 2) )
		{
			/* Wrong ACR LATENCY Config.Refuse to enable ART. */
			Cus_FLASH_ARTEnableFailed_Hook(acrReg);
			return;
		}

		if ( (acrReg & FLASH_ACR_PRFTEN_Msk) && (acrReg & FLASH_ACR_ICEN_Msk) && (acrReg & FLASH_ACR_DCEN_Msk) )
		{
			/* ART Accelerator has been Enabled. */
			return;
		}

		/* Enable Flash prefetch buffer and instruction cache (PRFTEN & ICEN & DCEN). */
		FLASH->ACR |= (FLASH_ACR_PRFTEN_Msk | FLASH_ACR_ICEN_Msk | FLASH_ACR_DCEN_Msk);
		__DSB();

		/* Check if the config has been written. */
		if ( !(FLASH->ACR & FLASH_ACR_PRFTEN_Msk) || !(FLASH->ACR & FLASH_ACR_ICEN_Msk) || !(FLASH->ACR & FLASH_ACR_DCEN_Msk) )
		{
			/* Something error happend? inform the upper layer. */
			Cus_FLASH_ARTEnableFailed_Hook(FLASH->ACR);
			return;
		}

		/* Success. Normal back. */
	}



	Cus_Flash_State_t 
	Cus_Flash_EraseSector( const Cus_Flash_Sector_t *pSector )
	{
		if ( !pSector )		return CUS_FLASH_PARAMETER;		/* Invalid parameter. */

		uint32_t timeout = Cus_Flash_GetSpinCount(300);
		while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout )
		{
			__nop();
		}
		if ( !timeout )
		{
			/* Timeout waiting for Flash BSY bit to clear. Bus is still busy. */
			Cus_FLASH_EraseSectorFailed_Hook(pSector);
			return CUS_FLASH_BUSY;
		}

		Cus_Flash_Unlock();

		/* Set FLASH_CR SER bit. */
		FLASH->CR |= FLASH_CR_SER_Msk;

		uint32_t snbVal = 0;
		switch (gsc_secTotalCount)
		{
			/* Directly write secIndex to SNB. For 12-sector devices, indices 0~11 map 1:1 to hardware encoding. */ 
			case 6:
			case 8:
			case 12:	snbVal = pSector->secIndex;	break;

			/* Physical sectors 12~23 map to SNB values 16~27 (offset +4), because SNB 12~15 are reserved. */ 
			case 24:	snbVal = (pSector->secIndex >= 12) ? (pSector->secIndex + 4) : (pSector->secIndex);

			/* Unexpected sector index. Notify upper layer via hook and return. */
			default:	goto ERROR;
		}

		/* Write SectorIndex which waiting for erasing to FLASH_CR SNB. */
		FLASH->CR |= (snbVal << FLASH_CR_SNB_Pos);

		/* Set FLASH_CR STRT bit. */
		FLASH->CR |= FLASH_CR_STRT_Msk;

		/* Wait for BSY to clear (erase finished). */
		timeout = Cus_Flash_GetSpinCount(3000);		/* about 3s. */
		while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout )
		{
			__nop();
		}
		if ( !timeout )
		{
			/* Error! BSY clear timeout. */
			goto ERROR;
		}

		/* Normal back. */
		Cus_Flash_Lock();
		return CUS_FLASH_OK;

	ERROR:
		Cus_Flash_Lock();
		Cus_FLASH_EraseSectorFailed_Hook(pSector);
		return CUS_FLASH_ERROR;
	}



	uint8_t 
	Cus_Flash_EraseSectors( const Cus_Flash_Sector_t *pSector, uint8_t eNum )
	{
		if ( !pSector || !eNum )		return 0;

		uint8_t Remain = Cus_Flash_GetRemainSectors(pSector->secIndex);
		if ( eNum > (Remain + 1) )
		{
			/* Requested erase count exceeds remaining sectors. Return. */
			return 0;
		}

		/* Erasing multiple sectors. */
		Cus_Flash_State_t hReturn = CUS_FLASH_OK;
		uint8_t offset = 0;
		uint8_t waitErased = eNum;
		while( (hReturn == CUS_FLASH_OK) && waitErased-- )
		{
			const Cus_Flash_Sector_t *currentSec = Cus_Flash_GetSectorbyIndex(pSector->secIndex + offset++);
			hReturn = Cus_Flash_EraseSector(currentSec);
		}
		if ( (waitErased != 0) && (hReturn != CUS_FLASH_OK) )
		{
			/* Something error happend while erasing.Return with Successfully erased count. */
			return (eNum - waitErased);
		}

		/* Successfully erased all sectors. Normal back. */
		return eNum;
	}



	Cus_Flash_State_t 
	Cus_Flash_WriteSector( const Cus_Flash_SecReq_t *pRequest )
	{
		if ( !pRequest )	return CUS_FLASH_PARAMETER;

		if ( ((pRequest->pSector->secStartAddr + pRequest->Offset) & 0x03) != 0 )
		{
			/* StartAddr + Offset must aligned to 4 Bytes. */
			return CUS_FLASH_ALIGNED_ERR;
		}

		uint32_t timeout = Cus_Flash_GetSpinCount(300);
		while( (FLASH->SR & FLASH_SR_BSY_Msk) && --timeout )
		{
			__nop();
		}
		if ( !timeout )
		{
			/* Timeout waiting for Flash BSY bit to clear. Bus is still busy. */
			return CUS_FLASH_BUSY;
		}

		/* Check. Whether the program region valid to be written. */
		uint32_t waitCheckBytes;
		#if (CUS_FLASH_USE_MANAGER)
			waitCheckBytes = sizeof(desc_t) + pRequest->bufSize;
		#else 
			waitCheckBytes = pRequest->bufSize;
		#endif /* CUS_FLASH_USE_MANAGER */

		uint8_t *pData = (uint8_t *)(pRequest->pSector->secStartAddr + pRequest->Offset);
		if ( waitCheckBytes <= CUS_CHECK_THRESHOLD )
		{
			/* Full exhaustive check for small block size (≤ CUS_CHECK_THRESHOLD). */
			for( uint32_t count = 0; count < waitCheckBytes; count++ )
			{
				if ( pData[count] != 0xFF )
				{
					/* 
						Target area not fully erased (non-0xFF detected). Reject write to prevent data corruption.
						Notify upper layer, and return. 
					*/
					goto NOT_ERASED;
				}
			}
		}
		else 
		{
			/* Sparse spot check: sample first 4B, last 4B, and every 128B in between. */
			/* First 4 Bits Check. */
			for ( uint8_t i = 0; i < 4; i++ )
				if ( pData[i] != 0xFF )  goto NOT_ERASED;		
			
			/* Middle Spot Check. */
			for ( uint32_t offset = 128; offset < waitCheckBytes - 4; offset += CUS_CHECK_INTERVAL )
				if ( pData[offset] != 0xFF ) goto NOT_ERASED;

			/* Last 4 Bits Check. */
			for ( int i = 0; i < 4; i++ )
        		if ( pData[waitCheckBytes - 4 + i] != 0xFF ) goto NOT_ERASED;
		}

		Cus_Flash_Unlock();

		/* Set FLASH_CR PG bit. */
		FLASH->CR |= FLASH_CR_PG_Msk;

		/* Enable x32 Program. PSIZE=0b10 */
		FLASH->CR = ((FLASH->CR & ~FLASH_CR_PSIZE_Msk) | (0x02UL << FLASH_CR_PSIZE_Pos));

		uint32_t dataBlockStartAddr = (pRequest->pSector->secStartAddr + pRequest->Offset);
		#if (CUS_FLASH_USE_MANAGER)
			/* Manager feature. Prepare the data control block. */
			desc_t title = { 0 };
			title.Magic = CUS_MANAGER_MAGIC;
			title.validFlag = 0;	/* 0=Data Valid. */
			title.StartAddr = (pRequest->pSector->secStartAddr + pRequest->Offset + sizeof(desc_t));
			title.Size = pRequest->bufSize;
			title.Type = pRequest->dataType;
			memcpy(title.Desc, pRequest->desc, sizeof(pRequest->desc));

			/* Update data block write addr. */
			dataBlockStartAddr = title.StartAddr;

			/* Write the control block. (Word) */
			uint32_t *src_ptr = (uint32_t *)&title;
			volatile uint32_t *desc_ptr = (volatile uint32_t *)(pRequest->pSector->secStartAddr + pRequest->Offset);
			uint8_t wordCount = sizeof(desc_t) / 4;
			
			for( uint8_t index = 0; index < wordCount; index++ )
			{
				desc_ptr[index] = src_ptr[index];

				/* Wait BSY clear and check Status Register. */
				if ( Cus_Flash_WaitBusy(50) != CUS_FLASH_OK )	goto PG_ERR;
			}

			/* Verify the control block if be written correctly. */
			if ( !Cus_Flash_VerifyBuffer((pRequest->pSector->secStartAddr + pRequest->Offset), (uint8_t *)src_ptr, sizeof(desc_t)) )
				goto VERIFY_ERR;
		#endif /* CUS_FLASH_USE_MANAGER */

		uint16_t dataCount_W = 0;
		uint8_t remainWaitToBeHandle = 0;
		if ( (pRequest->bufSize % 4) == 0 )
		{
			/* Buffer size is word-aligned. No tail bytes (1~3) need to be processed. */
			dataCount_W = pRequest->bufSize / 4;
		}
		else 
		{
			/* Buffer size is not word-aligned. Tail bytes (1~3) need to be handled. */
			dataCount_W = ((pRequest->bufSize / 4) + 1);
			remainWaitToBeHandle = 1;
		}

		/* Write user data. */
		volatile uint32_t *user = (volatile uint32_t *)dataBlockStartAddr;
		for( uint16_t index = 0; index < dataCount_W; index++ )
		{
			if ( remainWaitToBeHandle && (index == (dataCount_W - 1)) )
			{
				/* Handle the last 1 ~ 3 Bytes. */
				uint8_t remainByteNum = (pRequest->bufSize % 4);
				uint8_t remainStartIndex = (pRequest->bufSize - remainByteNum);
				uint32_t LastWord = 0;
				switch (remainByteNum)
				{
					case 1: memcpy(&LastWord, &pRequest->pBuffer[remainStartIndex], 1); LastWord |= 0xFFFFFF00UL; break;
					case 2: memcpy(&LastWord, &pRequest->pBuffer[remainStartIndex], 2); LastWord |= 0xFFFF0000UL; break;
					case 3: memcpy(&LastWord, &pRequest->pBuffer[remainStartIndex], 3); LastWord |= 0xFF000000UL; break;

					default:  goto PG_ERR;
				}

				/* Write the last word. */
				*user = LastWord;

				/* Wait BSY clear and check Status Register. */
				if ( Cus_Flash_WaitBusy(50) != CUS_FLASH_OK )	goto PG_ERR;

				goto SUCCESS;
			}

			uint32_t word;
			memcpy(&word, &pRequest->pBuffer[index * 4], 4);
			*user = word;

			/* Wait BSY clear and check Status Register. */
			if ( Cus_Flash_WaitBusy(50) != CUS_FLASH_OK )	goto PG_ERR;

			user++;
		}

	SUCCESS:	
		/* Verify The data block. */
		if ( !Cus_Flash_VerifyBuffer(dataBlockStartAddr, pRequest->pBuffer, pRequest->bufSize) )	goto VERIFY_ERR;

		Cus_Flash_Lock();
		return CUS_FLASH_OK;

	NOT_ERASED:
		Cus_FLASH_WriteSectorFailed_Hook(pRequest->pSector);
		return CUS_FLASH_NOT_ERASED;

	PG_ERR:
		Cus_Flash_Lock();
		Cus_FLASH_WriteSectorFailed_Hook(pRequest->pSector);
		return CUS_FLASH_ERROR;

	VERIFY_ERR:
		Cus_Flash_Lock();
		Cus_FLASH_VerifyBufferFailed_Hook(dataBlockStartAddr, pRequest->pBuffer, pRequest->bufSize);
		return CUS_FLASH_VERIFY_ERR;
	}

#endif /* (FLASH_TYPEERASE_SECTORS) && (DEVICE_STM32F4xx) */



