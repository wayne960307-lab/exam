# exam
├── Core                                  # 使用者核心原始碼與標頭檔目錄
│   ├── Inc                               # 核心標頭檔目錄 (*.h)
│   │   ├── main.h                        # 專案全域定義、自訂資料型態、引腳別名宣告與周邊 Handle 導出
│   │   ├── stm32g4xx_hal_conf.h          # HAL 模組啟用設定檔 (管理 TIM、GPIO、RCC 等模組的巨集開關)
│   │   └── stm32g4xx_it.h                # 中斷服務常式 (ISR) 與異常處理函式原型宣告
│   └── Src                               # 核心程式碼目錄 (*.c)
│       ├── main.c                        # 主程式進入點：包含系統初始化、非阻塞狀態機、PWM 呼吸燈演算法、蜂鳴器佇列驅動與 EXTI 回呼
│       ├── stm32g4xx_hal_msp.c           # MCU Support Package：低階硬體初始化 (GPIO、NVIC、TIM 時鐘與引腳複用對應)
│       ├── stm32g4xx_it.c                # 核心中斷常式實作檔：包含 SysTick_Handler、EXTI9_5_IRQHandler 與 EXTI15_10_IRQHandler
│       └── system_stm32g4xx.c            # CMSIS 系統底層檔：晶片啟動向量、SystemInit() 與 SystemCoreClock 變數維護
├── Drivers                               # 硬體抽象層與架構函式庫
│   ├── CMSIS                             # ARM Cortex-M4 核心微控制器軟體介面標準函式庫
│   │   ├── Device                        # STMicroelectronics 晶片特定暫存器定義與中斷向量表
│   │   │   └── ST
│   │   │       └── STM32G4xx
│   │   │           └── Include           # 包含 stm32g474xx.h (暫存器結構映射) 與 system_stm32g4xx.h
│   │   └── Include                       # ARM 核心架構暫存器標頭檔 (core_cm4.h、cmsis_gcc.h 等)
│   └── STM32G4xx_HAL_Driver              # ST 官方提供的硬體抽象層驅動程式庫
│       ├── Inc                           # HAL 驅動標頭檔
│       │   ├── stm32g4xx_hal.h           # HAL 核心設定與延遲/計時相關 API
│       │   ├── stm32g4xx_hal_cortex.h    # NVIC 與 SysTick 中斷控制 API
│       │   ├── stm32g4xx_hal_dma.h       # 直接記憶體存取控制器 API
│       │   ├── stm32g4xx_hal_exti.h      # 外部中斷控制器 API
│       │   ├── stm32g4xx_hal_flash.h     # 內部 Flash 快閃記憶體控制 API
│       │   ├── stm32g4xx_hal_gpio.h      # 通用輸出入腳位模式與中斷設定 API
│       │   ├── stm32g4xx_hal_pwr.h       # 電源控制與電壓等級設定 API
│       │   ├── stm32g4xx_hal_rcc.h       # 復位與時鐘控制 (Reset & Clock Control) API
│       │   ├── stm32g4xx_hal_tim.h       # 定時器基礎、輸入捕獲與 PWM 輸出 API
│       │   └── stm32g4xx_hal_tim_ex.h    # 高級定時器 (TIM8) 煞車死區與額外特性設定 API
│       └── Src                           # HAL 驅動實作檔
│           ├── stm32g4xx_hal.c           # HAL 初始架構、系統滴答時鐘計時驅動
│           ├── stm32g4xx_hal_cortex.c    # Cortex-M4 優先級群組與中斷啟用控制實作
│           ├── stm32g4xx_hal_dma.c       # DMA 資料流通道傳輸實作
│           ├── stm32g4xx_hal_exti.c      # EXTI 線路事件清除與觸發設定實作
│           ├── stm32g4xx_hal_flash.c     # Flash 讀寫保護與等待週期調整實作
│           ├── stm32g4xx_hal_gpio.c      # GPIO 初始化、拉電阻、速度與中斷派發實作
│           ├── stm32g4xx_hal_pwr.c       # 晶片升壓模式 (Range 1 Boost) 調度實作
│           ├── stm32g4xx_hal_rcc.c       # PLL 鎖相環頻率計算與匯流排分頻實作
│           ├── stm32g4xx_hal_tim.c       # 計時器基本時脈計數與 PWM 啟動實作
│           └── stm32g4xx_hal_tim_ex.c    # TIM8 高級定時器 MOE 主輸出致能實作
├── exam.ioc                              # STM32CubeMX 晶片圖形化設定原始檔 (引腳映射、時鐘樹、周邊配置)
├── STM32G474RETX_FLASH.ld                # GNU GCC 鏈結腳本 (定義 Flash/SRAM 記憶體區塊邊界與堆疊位置)
├── startup_stm32g474xx.s                 # ARM 組合語言啟動檔 (中斷向量表、重置處理常式、C 執行環境初始化)
└── README.md                             # 專案完整說明文件 (硬體接線圖、狀態機設計架構與操作指南)
