#include "bsp_lcd.h"
#include "stm32f7xx_hal.h"

/* Timing RK043FN48H (Pixeltakt 9,6 MHz) */
#define LCD_HSYNC 41U
#define LCD_HBP   13U
#define LCD_HFP   32U
#define LCD_VSYNC 10U
#define LCD_VBP   2U
#define LCD_VFP   2U

#define LCD_DISP_PORT GPIOI      /* PI12: Display ein */
#define LCD_DISP_PIN  GPIO_PIN_12
#define LCD_BL_PORT   GPIOK      /* PK3: Hintergrundbeleuchtung */
#define LCD_BL_PIN    GPIO_PIN_3

static LTDC_HandleTypeDef hltdc;
static volatile uint32_t s_shown_fb = BSP_LCD_FB0_ADDR;

static void lcd_gpio_init(void)
{
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Alternate = GPIO_AF14_LTDC;

    g.Pin = GPIO_PIN_4;                                    /* PE4: B0 */
    HAL_GPIO_Init(GPIOE, &g);

    g.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;   /* PI9 VSYNC, PI10 HSYNC, PI14 CLK, PI15 R0 */
    HAL_GPIO_Init(GPIOI, &g);

    g.Pin = 0x0FFFU | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;      /* PJ0..PJ11 (R1-R7,G0-G4), PJ13..15 (B1-B3) */
    HAL_GPIO_Init(GPIOJ, &g);

    g.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 |
            GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;           /* PK0..2 G5-G7, PK4..6 B5-B7, PK7 DE */
    HAL_GPIO_Init(GPIOK, &g);

    g.Pin = GPIO_PIN_12;                                    /* PG12: B4 (AF9) */
    g.Alternate = GPIO_AF9_LTDC;
    HAL_GPIO_Init(GPIOG, &g);

    /* Steuerleitungen DISP und Backlight */
    GPIO_InitTypeDef c = {0};
    c.Mode = GPIO_MODE_OUTPUT_PP;
    c.Pull = GPIO_NOPULL;
    c.Speed = GPIO_SPEED_FREQ_LOW;
    c.Pin = LCD_DISP_PIN;
    HAL_GPIO_Init(LCD_DISP_PORT, &c);
    c.Pin = LCD_BL_PIN;
    HAL_GPIO_Init(LCD_BL_PORT, &c);
}

uint16_t *bsp_lcd_framebuffer(void)
{
    return (uint16_t *)s_shown_fb;
}

int bsp_lcd_show_buffer(uint32_t fb_addr)
{
    if (fb_addr == s_shown_fb) {
        return 0;
    }
    HAL_LTDC_SetAddress_NoReload(&hltdc, fb_addr, LTDC_LAYER_1);
    if (HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_VERTICAL_BLANKING) != HAL_OK) {
        return -1;
    }
    /* Das Register wird im naechsten Austastintervall uebernommen: darauf warten */
    uint32_t t0 = HAL_GetTick();
    while ((LTDC->SRCR & (LTDC_SRCR_VBR | LTDC_SRCR_IMR)) != 0U) {
        if (HAL_GetTick() - t0 > 100U) {
            return -2;
        }
    }
    s_shown_fb = fb_addr;
    return 0;
}

void bsp_lcd_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
    if (x >= BSP_LCD_WIDTH || y >= BSP_LCD_HEIGHT) {
        return;
    }
    if (x + w > BSP_LCD_WIDTH)  w = BSP_LCD_WIDTH - x;
    if (y + h > BSP_LCD_HEIGHT) h = BSP_LCD_HEIGHT - y;

    uint16_t *fb = bsp_lcd_framebuffer();
    for (uint32_t row = y; row < y + h; row++) {
        uint16_t *p = &fb[row * BSP_LCD_WIDTH + x];
        for (uint32_t i = 0; i < w; i++) {
            p[i] = color;
        }
    }
}

void bsp_lcd_fill(uint16_t color)
{
    bsp_lcd_fill_rect(0, 0, BSP_LCD_WIDTH, BSP_LCD_HEIGHT, color);
}

void bsp_lcd_backlight(int on)
{
    HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

int bsp_lcd_init(void)
{
    lcd_gpio_init();

    /* Pixeltakt: PLLSAI (VCO-Eingang 1 MHz) N=192 -> 192 MHz, /R=5 -> 38,4 MHz, /DivR=4 -> 9,6 MHz */
    RCC_PeriphCLKInitTypeDef clk = {0};
    clk.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
    clk.PLLSAI.PLLSAIN = 192;
    clk.PLLSAI.PLLSAIR = 5;
    clk.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
    clk.PLLSAI.PLLSAIQ = 2;
    clk.PLLSAIDivR = RCC_PLLSAIDIVR_4;
    if (HAL_RCCEx_PeriphCLKConfig(&clk) != HAL_OK) {
        return -1;
    }

    /* Framebuffer vor dem Einschalten schwarz machen */
    bsp_lcd_fill(BSP_COLOR_BLACK);

    __HAL_RCC_LTDC_CLK_ENABLE();
    __HAL_RCC_LTDC_FORCE_RESET();
    __HAL_RCC_LTDC_RELEASE_RESET();

    hltdc.Instance = LTDC;
    hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
    hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
    hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
    hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
    hltdc.Init.HorizontalSync     = LCD_HSYNC - 1U;
    hltdc.Init.VerticalSync       = LCD_VSYNC - 1U;
    hltdc.Init.AccumulatedHBP     = LCD_HSYNC + LCD_HBP - 1U;
    hltdc.Init.AccumulatedVBP     = LCD_VSYNC + LCD_VBP - 1U;
    hltdc.Init.AccumulatedActiveW = LCD_HSYNC + BSP_LCD_WIDTH + LCD_HBP - 1U;
    hltdc.Init.AccumulatedActiveH = LCD_VSYNC + BSP_LCD_HEIGHT + LCD_VBP - 1U;
    hltdc.Init.TotalWidth         = LCD_HSYNC + BSP_LCD_WIDTH + LCD_HBP + LCD_HFP - 1U;
    hltdc.Init.TotalHeigh         = LCD_VSYNC + BSP_LCD_HEIGHT + LCD_VBP + LCD_VFP - 1U;
    hltdc.Init.Backcolor.Blue  = 0;
    hltdc.Init.Backcolor.Green = 0;
    hltdc.Init.Backcolor.Red   = 0;
    if (HAL_LTDC_Init(&hltdc) != HAL_OK) {
        return -2;
    }

    LTDC_LayerCfgTypeDef layer = {0};
    layer.WindowX0 = 0;
    layer.WindowX1 = BSP_LCD_WIDTH;
    layer.WindowY0 = 0;
    layer.WindowY1 = BSP_LCD_HEIGHT;
    layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer.FBStartAdress = BSP_LCD_FB0_ADDR;
    layer.Alpha = 255;
    layer.Alpha0 = 0;
    layer.Backcolor.Blue = 0;
    layer.Backcolor.Green = 0;
    layer.Backcolor.Red = 0;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
    layer.ImageWidth = BSP_LCD_WIDTH;
    layer.ImageHeight = BSP_LCD_HEIGHT;
    if (HAL_LTDC_ConfigLayer(&hltdc, &layer, LTDC_LAYER_1) != HAL_OK) {
        return -3;
    }

    HAL_GPIO_WritePin(LCD_DISP_PORT, LCD_DISP_PIN, GPIO_PIN_SET);
    bsp_lcd_backlight(1);
    return 0;
}
