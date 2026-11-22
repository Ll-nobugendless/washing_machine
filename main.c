 #include "wash_machine.h"
#include "delay.h"

/*==========================================================================
 * STM32F103C8T6 智能洗衣机 - 主程序入口
 *
 * 系统概述：
 *   - 主控芯片：STM32F103C8T6（Cortex-M3，72MHz）
 *   - 洗涤模式：快洗10分钟 / 标准洗16分钟 / 强力洗20分钟
 *   - 水位选择：低 / 中（默认） / 高
 *   - 脱水时间：4分钟（每次洗涤结束后）
 *   - 通信接口：USART1语音（PA9/PA10）+ USART3蓝牙（PB10/PB11），波特率均为9600
 *   - 时基定时器：TIM2（1ms中断）
 *   - 电机控制：ENA=PA8、IN1=PA7、IN2=PA15（普通GPIO高低电平，不用PWM）
 *   - 显示：0.96寸OLED（128x64，软件I2C，PB8/PB9）
 *   - 按键：4个按键（电源K1、启动/暂停K2、水位K3、模式K4）
 *
 * 语音/蓝牙16进制指令表：
 *   0x01 启动/洗衣（默认标准洗）    0x10 标准洗 + 启动
 *   0x02 停止                      0x11 快洗 + 启动
 *   0x03 暂停                      0x13 强力洗 + 启动
 *   0x04 继续                      0x20/0x21/0x22 水位 低/中/高
 *   0x30 单独脱水                  0x31 漂洗
 *   0x40 退出                      0x41 关机
 *   0x60 洗十分钟 + 启动           0x61 洗十五分钟 + 启动
 *   0x62 洗二十分钟 + 启动         0x63 洗三十分钟 + 启动
 *   0x67~0x6F 洗1~9分钟 + 启动     0x70 洗十二分钟 + 启动
 *   0x64 设置脱水3分钟             0x65 设置脱水4分钟
 *   0x66 设置脱水5分钟             0x71/0x72 设置脱水1/2分钟
 *   0x76~0x7A 设置脱水6~10分钟     0x7B 设置脱水12分钟
 *
 * 注意事项：
 *   - 全局毫秒计时由TIM2的1ms中断提供，不是SysTick
 *   - SysTick只用于delay_ms()等阻塞式延时函数
 *   - 请确保 stm32f10x_it.c 中的 SysTick_Handler 不要操作 g_timer_ms
 *========================================================================*/

int main(void)
{
    /* 1. 系统时钟初始化（配置为72MHz） */
    SystemInit();

    /* 2. 延时函数初始化（基于SysTick，用于阻塞式延时） */
    delay_init(72);

    /* 3. 配置NVIC优先级分组（2位抢占优先级 + 2位子优先级） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 3.1 关闭JTAG，释放PA15/PB3/PB4作为普通GPIO使用
     *     电机IN2现在接在PA15（原为JTAG的JTDI脚），
     *     必须禁用JTAG（保留SWD调试）才能正常控制PA15 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* 4. 洗衣机系统初始化
          包括：电机、水泵、水位传感器、蜂鸣器、串口、
                 定时器、按键、OLED显示屏 */
    WashMachine_Init();

    /* 5. 上电蜂鸣提示（短响一声表示启动成功） */
    Beep_On();
    delay_ms(200);   /* 蜂鸣器响200毫秒 */
    Beep_Off();

    /* 6. 主循环（永不退出）
          不断调用 WashMachine_Process() 处理所有事务 */
    while (1)
    {
        WashMachine_Process();
    }
}
