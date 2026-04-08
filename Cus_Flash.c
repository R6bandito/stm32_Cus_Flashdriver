#include "Cus_Flash.h"


/* ****************************************************** */

bool Cus_Flash_CalibrateLatency( void );
void Cus_Flash_Unlock( void );
void Cus_Flash_Lock( void );
static uint32_t Cus_Flash_GetSpinCount( uint32_t ms );
Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size );
bool Cus_Flash_VerifyBuffer(uint32_t StartAddress, uint8_t *pData, uint32_t Size);
bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size );

#if defined(FLASH_TYPEERASE_PAGES) 
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

  void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage );
  void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage );
#endif // FLASH_TYPEERASE_PAGES

void Cus_FLASH_UnlockFailed_Hook( void );
void Cus_FLASH_LockFailed_Hook( void );
void Cus_FLASH_EraseFailed_Hook( void );
void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize );
/* ****************************************************** */


void Cus_Flash_Lock( void )
{
  uint32_t FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) != 0 )    return;   // 已经上锁. 直接返回.

  FLASH->CR = (FLASH->CR | (0x01UL << FLASH_CR_LOCK_Pos));

  FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) == 0 )
  {
    // 上锁失败. 依然处于解锁状态.
    Cus_FLASH_LockFailed_Hook();

    return;
  }
}



void Cus_Flash_Unlock( void )
{
  uint32_t FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) == 0 )  return;   // FLASH已经被解锁. 直接返回.

  FLASH->KEYR = FLASH_KEYR_KEY1;    // 写入 KEY1.

  FLASH->KEYR = FLASH_KEYR_KEY2;    // 写入 KEY2.

  FLASH_CR_RegTemp = FLASH->CR;

  if ( (FLASH_CR_RegTemp & FLASH_CR_LOCK_Msk) != 0 )    // 验证是否成功解锁.
  {
    // 解锁失败.
    Cus_FLASH_UnlockFailed_Hook();

    return;
  }
}



bool Cus_Flash_CalibrateLatency( void )
{
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
static uint32_t Cus_Flash_GetSpinCount( uint32_t ms )
{
  // 估算每个循环消耗的 CPU 周期数.
  const uint32_t cycles_per_loop = 5;

  uint32_t sysclk = SystemCoreClock;

  uint32_t total_cycles = (ms * sysclk) / 1000;

  return total_cycles / cycles_per_loop;
}



Cus_Flash_State_t Cus_Flash_WriteBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Buffer_Size )
{
  if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || !pData || Buffer_Size == 0 )   
      return CUS_FLASH_PARAMERR;

  if ( (StartAddress & 0x01UL) != 0 )   // 地址未半字对齐. 
  {
    return CUS_FLASH_PARAMERR;
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



bool Cus_Flash_VerifyBuffer( uint32_t StartAddress, uint8_t *pData, uint32_t Size )
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



bool Cus_Flash_IsErase( uint32_t StartAddress, uint32_t Size )
{
  if ( StartAddress < FLASH_BASE || StartAddress > FLASH_END_ADDR || Size == 0 )  return false;

  for( uint32_t i = 0; i < Size; i++ )
  {
    if ( *((uint8_t *)StartAddress + i) != 0xFF )   return false;
  }

  return true;
}



#if defined(FLASH_TYPEERASE_PAGES) 

  Cus_Flash_State_t Cus_Flash_ErasePage(uint32_t PageAddress)
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


  uint32_t Cus_Flash_GetPageAddress( uint32_t page_index )
  {
    if ( (FLASH_BASE + page_index * FLASH_BYTES_PER_PAGE) >= FLASH_END_ADDR )   return 0;

    return (FLASH_BASE + ( page_index * FLASH_BYTES_PER_PAGE ));
  }


  uint32_t Cus_Flash_GetPageStart( uint32_t Address )
  {
    if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return 0;
    
    if ( (Address % FLASH_BYTES_PER_PAGE) == 0 )  return 0;   // 已经属于整页地址. 空调用.

    return Cus_Flash_GetPageAddress( ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE) );
  }


  Cus_Flash_State_t Factory_GetPageControlBlock( Cus_Flash_Page_t **pPageOut )
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


  Cus_Flash_State_t Cus_Flash_WritePage( Cus_Flash_Page_t *pPage )
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


  static void PageStructure_Reset( Cus_Flash_Page_t *pPage )
  {
    if ( !pPage )   return;

    pPage->PageAddress = 0;
    memset(pPage->PageDataBuffer, 0, FLASH_BYTES_PER_PAGE);
  }


  static void PageStructure_Release( Cus_Flash_Page_t *pPage )
  {
    /* 注意！ 传入的Cus_Flash_Page_t *pPage 必须是经由 Factory_GetPageControlBlock 创建并初始化的.否则将会出现释放错误. */
    if ( !pPage )   return;

    memset(pPage, 0, sizeof(Cus_Flash_Page_t));

    free(pPage);
  }


  bool Cus_Flash_ReadOutPage( uint32_t PageAddress, uint8_t *pOutBuffer, int32_t Size )
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


  int16_t Cus_Flash_GetPageIndex( uint32_t Address )
  {
    if ( Address < FLASH_BASE || Address > FLASH_END_ADDR )   return -1;

    return ((Address - FLASH_BASE) / FLASH_BYTES_PER_PAGE);
  }


  uint16_t Cus_Flash_GetTotalPages( void )
  {
    return ( (FLASH_END_ADDR - FLASH_BASE) / FLASH_BYTES_PER_PAGE );
  }


  int32_t Cus_Flash_GetRemainPages( uint32_t PageAddress )
  {
    if ( PageAddress < FLASH_BASE || PageAddress > FLASH_END_ADDR || (PageAddress % FLASH_BYTES_PER_PAGE) != 0 )   
          return -1;

    return ( (FLASH_END_ADDR - PageAddress) / FLASH_BYTES_PER_PAGE );
  }


  uint16_t Cus_Flash_ErasePages( uint32_t PageStartAddress, uint16_t PageCount )
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


__weak void Cus_FLASH_UnlockFailed_Hook( void )
{
  for( ; ; );
}

__weak void Cus_FLASH_LockFailed_Hook( void )
{
  for( ; ; );
}

__weak void Cus_FLASH_EraseFailed_Hook( void )
{
  for( ; ; );
}

__weak void Cus_FLASH_PageStructMallocFailed_Hook( Cus_Flash_Page_t **ppPage )
{
  UNUSED(ppPage);
  for( ; ; );
}

__weak void Cus_FLASH_PageWriteFailed_Hook( Cus_Flash_Page_t *pPage )
{
  UNUSED(pPage);
  for( ; ; );
}

__weak  void Cus_FLASH_VerifyBufferFailed_Hook( uint32_t StartAddress, uint8_t *pData, uint32_t BufferSize )
{
  UNUSED(pData);
  UNUSED(BufferSize);
  for( ; ; );
}
