#ifndef LV_CONF_H
#define LV_CONF_H

/* ==================== ОСНОВНЫЕ НАСТРОЙКИ ==================== */
#define LV_COLOR_DEPTH          16
#define LV_COLOR_16_SWAP        0
#define LV_MEM_SIZE             (96U * 1024U)   // увеличено под UI

/* ==================== ЛОГИРОВАНИЕ ==================== */
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_TRACE

/* ==================== ШРИФТЫ (все, что используются в SquareLine) ==================== */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_22   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_34   1
#define LV_FONT_MONTSERRAT_44   1

/* ==================== ФУНКЦИИ, КОТОРЫЕ НУЖНЫ ТВОЕМУ UI ==================== */
#define LV_USE_ANIMATION        1
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK   1

#define LV_USE_ARC              1
#define LV_USE_BAR              1
#define LV_USE_LABEL            1
#define LV_USE_IMG              1
#define LV_USE_SLIDER           1
#define LV_USE_SWITCH           1
#define LV_USE_DROPDOWN         1
#define LV_USE_KEYBOARD         1

/* ==================== ОТКЛЮЧАЕМ НЕНУЖНОЕ (экономия памяти) ==================== */
#define LV_USE_ANIM_IN_OUT      0
#define LV_USE_DRAW_MASKS       0

#endif